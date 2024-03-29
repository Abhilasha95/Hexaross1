#include "teleop_joy.hpp"

TeleopJoy::TeleopJoy() {

	ROS_INFO("Constructor.");

	joy_sub = node.subscribe<sensor_msgs::Joy>("joy", 1, &TeleopJoy::joyCallback, this);

	for (int i=0; i<18; i++) {
    	createPublishers(node, i);
  	}
		
}

void TeleopJoy::joyCallback(const sensor_msgs::Joy::ConstPtr& joy) {

	if (joy->buttons[PS3_BUTTON_REAR_RIGHT_1]) {

		addZOffset = 0;
		goToHome();
	}

	if (joy->axes[PS3_AXIS_STICK_LEFT_LEFTWARDS] || joy->axes[PS3_AXIS_STICK_LEFT_UPWARDS]) {

		inputX = joy->axes[PS3_AXIS_STICK_LEFT_UPWARDS];
		inputY = -joy->axes[PS3_AXIS_STICK_LEFT_LEFTWARDS];

		newX = 30*inputX;
		newY = 30*inputY;
		newZ = 0;

		limitInputs(newX, newY);

		wX = newX - oldX;
		wY = newY - oldY;
		wZ = newZ - oldZ;

		x = oldX;
		y = oldY;
		z = 0;

		linDis = sqrt(pow(wX,2) + pow(wY,2));
		linDis = round(linDis);
		int h=40; //height
		int p = linDis; //distance
		float steps = 10;


		ROS_INFO("newX =  %f", newX);
		ROS_INFO("newY =  %f", newY);
		ROS_INFO("X =  %f", wX);
		ROS_INFO("Y =  %f", wY);
		ROS_INFO("linDis =  %f", linDis);


		for (float i=0; i<=linDis && linDis != 0; ) {

			//swing
			z = -1*(h/pow((p/2),2))*(pow((i - (p/2)),2)) + h;

			for (int j=0; j<3; j++) {
				getLegCoordinatesFromWorldCoordinates(swingLegs[j], x, y, z, pX, pY, pZ);	
				getAngleWithIK(pX,pY,pZ,q1,q2,q3);

				Q1.data = q1;
				Q2.data = q2;
				Q3.data = q3;

				pub[swingLegs[j]*3 - 3].publish(Q1);
				pub[swingLegs[j]*3 - 2].publish(Q2);
				pub[swingLegs[j]*3 - 1].publish(Q3);
			}
			ROS_INFO("x =  %f,  y =  %f, i =  %f,  z =  %f", x, y, i, z);

			//stance
			z=0;
			for (int j=0; j<3; j++) {
				getLegCoordinatesFromWorldCoordinates(stanceLegs[j], -x, -y, z, pX, pY, pZ);	
				getAngleWithIK(pX,pY,pZ,q1,q2,q3);

				Q1.data = q1;
				Q2.data = q2;
				Q3.data = q3;

				pub[stanceLegs[j]*3 - 3].publish(Q1);
				pub[stanceLegs[j]*3 - 2].publish(Q2);
				pub[stanceLegs[j]*3 - 1].publish(Q3);
			}

			oldX = -x;
			oldY = -y;
			oldZ = -z;

			i += (linDis/steps);
			x += wX/steps;
			y += wY/steps;

			ros::Duration(0.01).sleep();
		}

		ROS_INFO("oldX =  %f", oldX);
		ROS_INFO("oldY =  %f", oldY);

		// changing sequence
		int temp;
		for (int i=0; i<3; i++) {
			temp = swingLegs[i];
			swingLegs[i] = stanceLegs[i];
			stanceLegs[i] = temp;
		}

	}

	if (joy->buttons[PS3_BUTTON_CROSS_UP]) {
		addZOffset = 40;

		goToHome();
	}

	if (joy->buttons[PS3_BUTTON_CROSS_DOWN]) {
		addZOffset = -40;

		goToHome();
	}
	
}

void TeleopJoy::goToHome() {

		getAngleWithIK(0,0,0,q1,q2,q3);

		Q1.data = q1;
		Q2.data = q2;
		Q3.data = q3;

		pub[0].publish(Q1);
		pub[1].publish(Q2);
		pub[2].publish(Q3);

		pub[3].publish(Q1);
		pub[4].publish(Q2);
		pub[5].publish(Q3);

		pub[6].publish(Q1);
		pub[7].publish(Q2);
		pub[8].publish(Q3);

		pub[9].publish(Q1);
		pub[10].publish(Q2);
		pub[11].publish(Q3);

		pub[12].publish(Q1);
		pub[13].publish(Q2);
		pub[14].publish(Q3);

		pub[15].publish(Q1);
		pub[16].publish(Q2);
		pub[17].publish(Q3);  

		oldX = 0;
		oldY = 0;
		oldZ = 0;

}

void TeleopJoy::limitInputs(float& newX, float& newY) {

	if (newX>=0 && newX<10) 
		newX = 0;
	else if (newX>=10 && newX<20) 
		newX = 10;
	else if (newX>=20 && newX<30)
		newX = 20;
	else if (newX==30)
		newX = 30;
	else if (newX<=0 && newX>-10) 
		newX = -0;
	else if (newX<=-10 && newX>-20) 
		newX = -10;
	else if (newX<=-20 && newX>-30) 
		newX = -20;
	else if (newX==-30)
		newX = -30;
	else
		newX = 0;

	if (newY>=0 && newY<10) 
		newY = 0;
	else if (newY>=10 && newY<20) 
		newY = 10;
	else if (newY>=20 && newY<30)
		newY = 20;
	else if (newY==30)
		newY = 30;
	else if (newY<=0 && newY>-10) 
		newY = -0;
	else if (newY<=-10 && newY>-20) 
		newY = -10;
	else if (newY<=-20 && newY>-30) 
		newY = -20;
	else if (newY==-30)
		newY = -30;
	else
		newY = 0;

}

void TeleopJoy::getAngleWithIK(float px, float py, float pz, float& q1, float& q2, float& q3){
	float r;
	float s;
	float squad;
	float tmp;
	float omega;
	

	py = py + yOffset;
	pz = pz + zOffset + addZOffset;

	
	q1=-1*atan2(px,py);

	//Changing origin from Coxa Frame to Femur Frame
    float xF, yF, zF;
    
    tmp= pow(px,2)+pow(py,2);
    xF = sqrt(tmp) - C;
    yF = -pz;
    zF = 0;

	tmp=pow(xF,2)+pow(yF,2);
	s=sqrt(tmp);
	squad=pow(s,2);
	omega=atan2(yF,xF);
	

	tmp=((F*F)+squad-(T*T))/(2*F*s);
	q2=acos(tmp)+omega;
	
	tmp=((F*F)+(T*T)-squad)/(2*F*T);
	q3= -3.14159+acos(tmp);

	q3 = q3+0.6335545;    //adding 36.3 deg because of leg construction

}

void TeleopJoy::getLegCoordinatesFromWorldCoordinates(int legNumber, float pwX, float pwY, float pwZ, float& pX, float& pY, float& pZ){
		switch (legNumber){
			case 1:
				pX=pwX*cos(c_phi)-pwY*sin(c_phi);
				pY=pwX*sin(c_phi)+pwY*cos(c_phi);
				pZ=-pwZ;
			break;
			case 2:
				pX=-1*(pwX*cos(c_phi)+pwY*sin(c_phi));
				pY=-1*(-pwX*sin(c_phi)+pwY*cos(c_phi));
				pZ=-pwZ;
			break;
			case 3:
			    pX=pwX;
				pY=pwY;
				pZ=-pwZ;
			break;
			case 4:
				pX=-pwX;
				pY=-pwY;
				pZ=-pwZ;
			break;
			case 5:
				pX=pwX*cos(c_phi)+pwY*sin(c_phi);
				pY=-pwX*sin(c_phi)+pwY*cos(c_phi);
				pZ=-pwZ;
			break;
			case 6:
				pX=-1*(pwX*cos(c_phi)-pwY*sin(c_phi));
				pY=-1*(pwX*sin(c_phi)+pwY*cos(c_phi));
				pZ=-pwZ;
			break;
			default:
				ROS_INFO("Default called");
			break;
		}
}

void TeleopJoy::createPublishers(ros::NodeHandle &node, int num) {
      pub[num] = node.advertise<std_msgs::Float64>("/hexapod/" + jointNames[num] + "_position_controller/command", 1);
}

int main(int argc, char **argv)
{
	ros::init(argc, argv, "teleop_joy");
	ROS_INFO("Starting ps3 teleop converter, take care of your controller now...");
	TeleopJoy teleop_joy;
	ros::spin();
//    return 0;
}
