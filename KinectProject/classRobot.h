#pragma once

class robot {
public:
	//Robot states varriable
	enum RobotStates {
		rIdle,
		rInitial,
		rSayHi
	};

	void setRobotStates(RobotStates inputStates);

	RobotStates getRobotState(void);

private:
	RobotStates robotStates = rIdle;
};