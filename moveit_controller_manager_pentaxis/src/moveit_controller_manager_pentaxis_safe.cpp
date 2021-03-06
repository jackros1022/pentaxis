/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2013, Moises Estrada Casta/neda
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Moises Estrada Casta/neda nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

/** @Author Moises Estrada */

#include <ros/ros.h>
#include <moveit/controller_manager/controller_manager.h>
#include <sensor_msgs/JointState.h>
#include <pluginlib/class_list_macros.h>
#include <map>
#include <iostream>
#include "Galil.h"
#include <string>
#include "std_msgs/String.h"


namespace moveit_controller_manager_pentaxis
{ using namespace std;

/** @class
*  @brief Hardware driver class for the Galil DMC-4060
*/
class PentaxisControllerHandle : public moveit_controller_manager::MoveItControllerHandle {	
	private:
		float current_joint_state_pos[5];
	public:
	
	/** 
	*  @brief Ctor
	*/
	PentaxisControllerHandle(const std::string &name, ros::NodeHandle &nh, const std::vector<std::string> &joints) :
		moveit_controller_manager::MoveItControllerHandle(name), nh_(nh), joints_(joints) {
			pub_ = nh_.advertise<sensor_msgs::JointState>("fake_controller_joint_states", 10, false);
			if(joints.size() == 5)
				doHoming();
			for(int i = 0; i < 5; i++) {
				current_joint_state_pos[i] = 0;
			}
		}

	/** 
	*  @brief Homing squence for the Pentaxis Arm
	*/
	bool doHoming() {
		try {
			// Creates the "g" object.
			Galil g("192.168.1.2");  //Crea conexion
			// Prints and creates the connection
			ROS_INFO_STREAM( g.connection() );
			// Prints the library version being used
			ROS_INFO_STREAM( g.libraryVersion() );
			// Prints the current time
			ROS_INFO_STREAM( "MG TIME " << g.command("MG TIME") );
			// Downlaod the specified program to Galil
			g.programDownload("#HOMING\rMT 2, 2, 2, 2, 2, 2\rSP 25000,25000,25000,25000,25000,25000\r AC 256000,256000,256000,256000,256000,256000\r DC 256000,256000,256000,256000,256000,256000\r IF(_LFA=0)\r PRA= -10000\r  BG A\r ENDIF \r IF(_LFB=0)\r PRB= -20000\r BG B\r ENDIF;\r IF(_LRC=0)\r PRC=20000\r BG C\r ENDIF;\r IF(_LFD=0)\r PRD= -2000\r BG D\r ENDIF\r IF(_LFE=0)\r PRE= -2000\r BG E\r ENDIF\r IF(_LFF=0)\r PRF= -1000\r BG F\r ENDIF\r AM A,B,C,D,E,F\r PR 0,0,0,0,0,0\r JG 20000,5000,-25000,15000,7500,1000\rBG A,B,C,D,E,F\rAM A,B,C,D,E,F\rJG 0,0,0,0,0,0\rSP 20000,20000,20000, 20000, 16000,1000\rAC 100000,100000,100000, 100000, 70000,10000\rDC 100000,100000,100000, 100000, 70000,10000\rPR -88700, -21000, 78200, -105000, -89000, -2720\rBG A,B,C,E,F\rWT 800\rBG D\rAM A,B,C,D,E,F\rPR 0,0,0,0,0,0\rDP 0,0,0,0,0,0");
			// Executes the programs
			ROS_INFO_STREAM( g.command("XQ #HOMING") );
			return true;
		}  catch (string e) {
			cout << e;
			return false;
		}
	}
  
	void getJoints(std::vector<std::string> &joints) const {
		joints = joints_;
	}
	
	void printTrajectory(const moveit_msgs::RobotTrajectory &t, int joint_a) {
		int n_points = t.joint_trajectory.points.size();
		int n_joints = t.joint_trajectory.joint_names.size();
		cout << "Trejectory for joint:" << joint_a << endl;
		cout << "[TIME]\t[POSITIONS]\t[VELOCITIES]\t[ACCELERATIONS]" << endl;
		for(int i = 0; i < n_points; i++) {
			ROS_INFO_STREAM( t.joint_trajectory.points[i].time_from_start << " \t" << t.joint_trajectory.points[i].positions[joint_a] << "\t" );
		}
	}
	
  
  virtual bool sendTrajectory(const moveit_msgs::RobotTrajectory &t) {
	//JointManager jointManager(t);
	int n_points = t.joint_trajectory.points.size();
	int n_joints = t.joint_trajectory.joint_names.size();
	float diffs_pos[n_points][n_joints];
	//float diffs_vel[n_points-1][n_joints];
	int relative_pos_steps[n_points][n_joints];
	int vel_steps[n_points][n_joints];
	std::string pvt_cmd_head[5] = {"PVA", "PVB", "PVC", "PVD", "PVE"};
	std::ostringstream pvt_cmd;

	try {
		// Creates the "g" object.
		Galil g("192.168.1.2");  //Crea conexion
		// Prints and creates the connection
		ROS_INFO_STREAM( g.connection() );
		// Prints the library version being used
		ROS_INFO_STREAM( g.libraryVersion() );
		// Tells if the arm is being used.
		if(n_joints == 5) {
			for(int i = 0; i < n_joints; i++) {
				// 
				diffs_pos[0][i] = t.joint_trajectory.points[0].positions[i] - current_joint_state_pos[i];
			}
			for(int i = 0; i < n_joints; i++) {
				current_joint_state_pos[i] = t.joint_trajectory.points[n_points-1].positions[i];
			}
			for(int i = 0; i < n_joints; i++) {
				for(int j = 1; j < n_points; j++) {
					diffs_pos[j][i] = t.joint_trajectory.points[j].positions[i] - t.joint_trajectory.points[j-1].positions[i];
					if(i == 4) 
						diffs_pos[j][i] = (-1*diffs_pos[j][i] + diffs_pos[j][i-1])/2; //Position transmition compensation
				}
			}
			for(int i = 0; i < n_joints; i++) {
				for(int j = 0; j < n_points; j++) {
					relative_pos_steps[j][i] = (int) round(diffs_pos[j][i]*200000/(2*M_PI));
					if(i == 4) //Velocity transmition compensation
						vel_steps[j][i] = (int) round(t.joint_trajectory.points[j].velocities[i]*100000/(-2*M_PI));
					else
						vel_steps[j][i] = (int) round(t.joint_trajectory.points[j].velocities[i]*200000/(2*M_PI));
				}
			}
			ROS_INFO("Constructing the GALIL program");
			// Creating the Galil Command
			pvt_cmd << "#PVT\n";			
			pvt_cmd << pvt_cmd_head[0] << "=" << relative_pos_steps[0][0] << "," << vel_steps[0][0] << "," << "128;";
			pvt_cmd << pvt_cmd_head[1] << "=" << relative_pos_steps[0][1] << "," << vel_steps[0][1] << "," << "128\n";
			pvt_cmd << pvt_cmd_head[2] << "=" << relative_pos_steps[0][2] << "," << vel_steps[0][2] << "," << "128;";
			pvt_cmd << pvt_cmd_head[3] << "=" << relative_pos_steps[0][3] << "," << vel_steps[0][3] << "," << "128\n";
			pvt_cmd << pvt_cmd_head[4] << "=" << relative_pos_steps[0][4] << "," << vel_steps[0][4] << "," << "128\n";
			for(int i = 1; i < n_points; i++) {
				pvt_cmd << pvt_cmd_head[0] << "=" << relative_pos_steps[i][0] << "," << vel_steps[i][0] << ";";
				pvt_cmd << pvt_cmd_head[1] << "=" << relative_pos_steps[i][1] << "," << vel_steps[i][1] << ";";
				pvt_cmd << pvt_cmd_head[2] << "=" << relative_pos_steps[i][2] << "," << vel_steps[i][2] << "\n";
				pvt_cmd << pvt_cmd_head[3] << "=" << relative_pos_steps[i][3] << "," << vel_steps[i][3] << ";";
				pvt_cmd << pvt_cmd_head[4] << "=" << relative_pos_steps[i][4] << "," << vel_steps[i][4] << "\n";
			}
			pvt_cmd << "PVA=0,0,0;" << "PVB=0,0,0;" << "PVC=0,0,0;" << "PVD=0,0,0;" << "PVE=0,0,0\n";
			pvt_cmd << "BTABCDE\nMC\nMG \"DONE\" \nEN\n"; // Command complete
			ROS_INFO_STREAM(pvt_cmd.str());
			ROS_INFO("Downloading program to GALIL");
			g.programDownload(pvt_cmd.str()); // Sending the Command
			ROS_INFO("Executing program");
			g.command("XQ #PVT"); // Execcuting the Command
			// Wait X numer of miliseconds to complete motion
			ROS_INFO("Waiting for motors to stop moving");
			std::string completion = g.message( (int) round(t.joint_trajectory.points[n_points-1].time_from_start.toSec()*1000+500) );
			// Tells if the motors have stopped moving
			if(completion.compare(0,4,"DONE") == 0) {
				ROS_INFO("The motors have stopped moving.");
			}
			else
				return false;
			// Since there are no encoder, the returning JointState is the same as the input
			if (!t.joint_trajectory.points.empty()) {
			  	sensor_msgs::JointState js;
			  	js.header = t.joint_trajectory.header;
			  	js.name = t.joint_trajectory.joint_names;
			 	js.position = t.joint_trajectory.points.back().positions;
			  	js.velocity = t.joint_trajectory.points.back().velocities;
			 	js.effort = t.joint_trajectory.points.back().effort;
			  	pub_.publish(js);
    		}
		} else if(n_joints == 1) { // Tells if the gripper is being used
			float gripper_pos;
			//check if command is to close or open
			if(t.joint_trajectory.points[0].positions[0] > t.joint_trajectory.points[n_points-1].positions[0]) {
				// to close
				ROS_INFO("Closing the gripper");
				g.command("JGF= 1500");
				g.command("BG F");
				gripper_pos = -0.020;
			} else {
				ROS_INFO("Opening the gripper");
				g.command("PAF= 0");
				g.command("BG F");
				gripper_pos = 0.0;
			}
			if (!t.joint_trajectory.points.empty()) {
			  	sensor_msgs::JointState js;
			  	js.header = t.joint_trajectory.header;
			  	js.name = t.joint_trajectory.joint_names;
				js.position = t.joint_trajectory.points.back().positions;
			 	js.position[0] = gripper_pos;
			  	js.velocity = t.joint_trajectory.points.back().velocities;
			 	js.effort = t.joint_trajectory.points.back().effort;
			  	pub_.publish(js);
    		}
		}
    	return true;
	} catch (string e) {
		cout << e;
		return false;
	}
    
  }
  
  virtual bool cancelExecution() {   
    ROS_INFO("Fake trajectory execution cancel");
    return true;
  }
  
  virtual bool waitForExecution(const ros::Duration &) {
    return true;
  }
  
  virtual moveit_controller_manager::ExecutionStatus getLastExecutionStatus() {
    return moveit_controller_manager::ExecutionStatus(moveit_controller_manager::ExecutionStatus::SUCCEEDED);
  }
  
private:
  ros::NodeHandle nh_;
  std::vector<std::string> joints_;
  ros::Publisher pub_;
};

/**
* @class
* @brief Manages the controllers. There is only one, but several may be implemented.
*/
class MoveItControllerManagerPentaxis : public moveit_controller_manager::MoveItControllerManager
{
public:

  MoveItControllerManagerPentaxis() : node_handle_("~")
  {
    if (!node_handle_.hasParam("controller_list"))
    {
      ROS_ERROR_STREAM("MoveItFakeControllerManager: No controller_list specified.");
      return;
    }
    
    XmlRpc::XmlRpcValue controller_list;
    node_handle_.getParam("controller_list", controller_list);
    if (controller_list.getType() != XmlRpc::XmlRpcValue::TypeArray)
    {
      ROS_ERROR("MoveItFakeControllerManager: controller_list should be specified as an array");
      return;
    }
    
    /* actually create each controller */
    for (int i = 0 ; i < controller_list.size() ; ++i)
    {
      if (!controller_list[i].hasMember("name") || !controller_list[i].hasMember("joints"))
      {
        ROS_ERROR("MoveItFakeControllerManager: Name and joints must be specifed for each controller");
        continue;
      }
      
      try
      {
        std::string name = std::string(controller_list[i]["name"]);
        
        if (controller_list[i]["joints"].getType() != XmlRpc::XmlRpcValue::TypeArray)
        {
          ROS_ERROR_STREAM("MoveItFakeControllerManager: The list of joints for controller " << name << " is not specified as an array");
          continue;
        }
        std::vector<std::string> joints;
        for (int j = 0 ; j < controller_list[i]["joints"].size() ; ++j)
          joints.push_back(std::string(controller_list[i]["joints"][j]));

        controllers_[name].reset(new PentaxisControllerHandle(name, node_handle_, joints));
      }
      catch (...)
      {
        ROS_ERROR("MoveItFakeControllerManager: Unable to parse controller information");
      }
    }
  }
  
  virtual ~MoveItControllerManagerPentaxis()
  {
  }

  /**
   * @brief Get a controller, by controller name (which was specified in the controllers.yaml
   */
  virtual moveit_controller_manager::MoveItControllerHandlePtr getControllerHandle(const std::string &name)
  {
    std::map<std::string, moveit_controller_manager::MoveItControllerHandlePtr>::const_iterator it = controllers_.find(name);
    if (it != controllers_.end())
      return it->second;
    else
      ROS_FATAL_STREAM("No such controller: " << name);
    return moveit_controller_manager::MoveItControllerHandlePtr();
  }

  /**
   * @brief Get the list of controller names.
   */
  virtual void getControllersList(std::vector<std::string> &names)
  {
    for (std::map<std::string, moveit_controller_manager::MoveItControllerHandlePtr>::const_iterator it = controllers_.begin() ; it != controllers_.end() ; ++it)
      names.push_back(it->first);
    ROS_INFO_STREAM("Returned " << names.size() << " controllers in list");
  }

  /**
   * @brief This plugin assumes that all controllers are already active -- and if they are not, well, it has no way to deal with it anyways!
   */
  virtual void getActiveControllers(std::vector<std::string> &names)
  {
    getControllersList(names);
  }

  /**
   * @brief Controller must be loaded to be active, see comment above about active controllers...
   */
  virtual void getLoadedControllers(std::vector<std::string> &names)
  {
    getControllersList(names);
  }

  /**
   * @brief Get the list of joints that a controller can control.
   */
  virtual void getControllerJoints(const std::string &name, std::vector<std::string> &joints) {
	std::map<std::string, moveit_controller_manager::MoveItControllerHandlePtr>::const_iterator it = controllers_.find(name);
    if (it != controllers_.end())
    {
      static_cast<PentaxisControllerHandle*>(it->second.get())->getJoints(joints);
    }
    else
    {
      ROS_WARN("The joints for controller '%s' are not known. Perhaps the controller configuration is not loaded on the param server?", name.c_str());
      joints.clear();
    }
  }

  /**
   * @brief Controllers are all active and default.
   */
  virtual moveit_controller_manager::MoveItControllerManager::ControllerState getControllerState(const std::string &name)
  {
    moveit_controller_manager::MoveItControllerManager::ControllerState state;
    state.active_ = true;
    state.default_ = true;
    return state;
  }

  /** @brief Cannot switch our controllers */
  virtual bool switchControllers(const std::vector<std::string> &activate, const std::vector<std::string> &deactivate) { return false; }

protected:

  ros::NodeHandle node_handle_;
  std::map<std::string, moveit_controller_manager::MoveItControllerHandlePtr> controllers_;
};

} // end namespace moveit_fake_controller_manager

PLUGINLIB_EXPORT_CLASS(moveit_controller_manager_pentaxis::MoveItControllerManagerPentaxis,                      moveit_controller_manager::MoveItControllerManager);

