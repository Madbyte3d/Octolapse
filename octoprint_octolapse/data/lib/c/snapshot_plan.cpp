////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Octolapse - A plugin for OctoPrint used for making stabilized timelapse videos.
// Copyright(C) 2019  Brad Hochgesang
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// This program is free software : you can redistribute it and/or modify
// it under the terms of the GNU Affero General Public License as published
// by the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.See the
// GNU Affero General Public License for more details.
//
// You should have received a copy of the GNU Affero General Public License
// along with this program.If not, see the following :
// https ://github.com/FormerLurker/Octolapse/blob/master/LICENSE
//
// You can contact the author either through the git - hub repository, or at the
// following email address : FormerLurker@pm.me
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#include "snapshot_plan.h"
#include <iostream>
#include "logging.h"
snapshot_plan::snapshot_plan()
{
	file_line = 0;
	file_gcode_number = 0;
	total_travel_distance = 0;
	saved_travel_distance = 0;
	triggering_command_type = trigger_position::unknown; // unknown
	has_initial_position = false;
}


PyObject * snapshot_plan::build_py_object(std::vector<snapshot_plan> &p_plans)
{
	
	PyObject *py_snapshot_plans = PyList_New(0);
	if (py_snapshot_plans == NULL)
	{
		PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.build_py_object: Unable to create SnapshotPlans PyList object.");
		return NULL;
	}
	
	// Create each snapshot plan
	for (unsigned int plan_index = 0; plan_index < p_plans.size(); plan_index++)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Building Plan.");
		PyObject * py_snapshot_plan = p_plans[plan_index].to_py_object();
		if (py_snapshot_plan == NULL)
		{
			//PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.build_py_object: Unable to convert the snapshot plan to a PyObject.");
			return NULL;
		}
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding plan to list of plans.");
		bool success = !(PyList_Append(py_snapshot_plans, py_snapshot_plan) < 0); // reference to pSnapshotPlan stolen
		if (!success)
		{
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.build_py_object: Unable to append the snapshot plan to the snapshot plan list.");
			return NULL;
		}
		// Need to decref after PyList_Append, since it increfs the PyObject
		Py_DECREF(py_snapshot_plan);
		//std::cout << "py_snapshot_plan refcount = " << py_snapshot_plan->ob_refcnt << "\r\n";
	}
	
	return py_snapshot_plans;
}

PyObject * snapshot_plan::to_py_object()
{
	PyObject* py_triggering_command;
	
	if (triggering_command.is_empty)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "No triggering command for plan.");
		py_triggering_command = Py_None;
		Py_IncRef(py_triggering_command);
	}
	else
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding the triggering command.");
		py_triggering_command = triggering_command.to_py_object();
		if (py_triggering_command == NULL)
		{
			PyErr_Print();
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to convert the triggering_command to a PyObject.");
			return NULL;
		}
	}
	

	PyObject* py_start_command = NULL;
	if (start_command.is_empty)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "No start command.");
		py_start_command = Py_None;
		Py_IncRef(Py_None);
	}
	else
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding the start command.");
		py_start_command = start_command.to_py_object();
		if (py_start_command == NULL)
		{
			PyErr_Print();
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to convert the start_command to a PyObject.");
			return NULL;
		}
	}

	PyObject * py_initial_position;
	if (!has_initial_position)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "No initial position.");
		py_initial_position = Py_None;
		Py_IncRef(py_initial_position);
	}
	else
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding the initial position.");
		py_initial_position = initial_position.to_py_tuple();
		if (py_initial_position == NULL)
		{
			PyErr_Print();
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to create InitialPosition PyObject.");
			return NULL;
		}
	}

	PyObject * py_steps = PyList_New(0);
	if (py_steps == NULL)
	{
		PyErr_Print();
		PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to create a PyList object to hold the snapshot plan steps.");
		return NULL;
	}
	for (unsigned int step_index = 0; step_index < steps.size(); step_index++)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding snapshot step.");
		// create the snapshot step object with build
		PyObject * py_step = steps[step_index].to_py_object();
		if (py_step == NULL)
		{
			PyErr_Print();
			return NULL;
		}
		bool success = !(PyList_Append(py_steps, py_step) < 0); // reference to pSnapshotPlan stolen
		if (!success)
		{
			PyErr_Print();
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to append the snapshot plan step to the snapshot plan step list.");
			return NULL;
		}
		// Need to decref after PyList_Append, since it increfs the PyObject
		// Todo: evaluate the effect of this 
		Py_DECREF(py_step);
		//std::cout << "py_step refcount = " << py_step->ob_refcnt << "\r\n";

	}

	
	PyObject * py_return_position;
	if (return_position.is_empty)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "No return position.");
		py_return_position = Py_None;
		Py_IncRef(py_return_position);
	}
	else
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding return position.");
		py_return_position = return_position.to_py_tuple();
		if (py_return_position == NULL)
		{
			PyErr_Print();
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to convert the return position to a tuple.");
			return NULL;
		}
	}
	
	PyObject* py_end_command;
	if (end_command.is_empty)
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "No end command.");
		py_end_command = Py_None;
		Py_IncRef(py_end_command);
	}
	else
	{
		octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Adding the end command.");
		py_end_command = end_command.to_py_object();
		if (py_end_command == NULL)
		{
			PyErr_Print();
			PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to convert the end_command to a PyObject.");
			return NULL;
		}
	}

	octolapse_log(octolapse_log::SNAPSHOT_PLAN, octolapse_log::VERBOSE, "Converting plan to py object.");
	PyObject *py_snapshot_plan = Py_BuildValue(
		"llddOOOOOO",
		file_line,
		file_gcode_number,
		total_travel_distance,
		saved_travel_distance,
		py_triggering_command,
		py_start_command,
		py_initial_position,
		py_steps,
		py_return_position,
		py_end_command
	);
	if (py_snapshot_plan == NULL)
	{
		PyErr_Print();
		PyErr_SetString(PyExc_ValueError, "Error executing SnapshotPlan.to_py_object: Unable to create SnapshotPlan PyObject.");
		return NULL;
	}
	
	Py_DECREF(py_triggering_command);
	Py_DECREF(py_initial_position);
	Py_DECREF(py_return_position);
	Py_DECREF(py_steps);
	Py_DECREF(py_start_command);
	Py_DECREF(py_end_command);
	
	return py_snapshot_plan;
}