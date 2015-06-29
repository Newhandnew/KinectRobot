#include "classRobot.h"


void robot::setRobotStates(RobotStates inputStates) {
	robotStates = inputStates;
}
robot::RobotStates robot::getRobotState(void) {
	return robotStates;
}