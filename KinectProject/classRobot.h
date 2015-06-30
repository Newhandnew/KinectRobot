#pragma once

class robot {
public:
	//Robot states varriable
	bool fManual;
	enum RobotStates {
		rIdle,
		rInitial,
		rSayHi,
		rManual
	};

	void setRobotStates(RobotStates inputStates);

	RobotStates getRobotState(void);

private:
	RobotStates robotStates = rIdle;
};