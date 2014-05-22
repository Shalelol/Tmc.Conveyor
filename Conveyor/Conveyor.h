 class Conveyor
{
	private:
		
		int state, speed, position; // double check the number of states


		/*
		unsigned char acInput[BUFFER_SIZE];
		unsigned char acTmp[BUFFER_SIZE];
		unsigned char acOutput[BUFFER_SIZE];

		int    iLen;

		int    i;*/

		 int initialise(void);

	public:
		 int getState(void);
		 int getPosition(void);
		 int getSpeed(void);
		 bool stop(void);
		 bool start(void);
		 bool emergencyStop(void);
};

