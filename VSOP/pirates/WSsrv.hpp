#ifndef _wssrv_
#define _wssrv_

#include <iostream>
#include <vector>
#include <map>

#include "timer.hpp"

#include "WebServerWS.hpp"
#include <solarSystem.hpp>
#include <skymap.hpp>
#include <map.hpp>

#include <tcp_client.hpp>




class WSsrv : public WebServerWS {


	friend class SkyGrid;
	
//	TConnection connession;
//	TQuery mysql;

	class clientData  * client_data;
	
	Map * moonGrid ;
	Map * earthGrid ;
	
	TCPClient geoLoc;

	void do_disconect ( Client * client ) ;
	bool do_request ( Client * ) ;
	void do_sendLoop ( Client * client ) ;
	void do_before_sendLoop ( Client * client ) ;
	
public :

	virtual void handleClient (  int sockfd, std::string buffer   ) ;
	void initDb () ;

	WSsrv ( unsigned int port, std::string & web_dir );
	virtual ~WSsrv ( );


};


class clientData {
		
	
		public :
		
		const double DEF_FACT = 1 ;
		const short DEF_REFRESH = 0;
		const double DEF_SCALE = 1,
			DEF_SIMULATION = 1,
			DEF_UP_X = 0,
			DEF_UP_Y =0,
			DEF_UP_Z = 1,
			DEF_FOV = 45,
			DEF_ZOOM = 1,
			DEF_NEAR = 0.000003,
			DEF_FAR = 50,
			DEF_MOVE_LAT = 0,
			DEF_MOVE_LNG = 0;
		
		WSsrv * parent ;
		SolarSystem * system;
		int sockfd ;
		
		Timer timer;
		Timer timer_result ;
		
		Date ref_date;
 		short	refresh ;	// secondi;
 		double scale ;
 		double rotation ;
		double moon_rotation = 0;
  		double simulation ;
  		
		SkyGrid skyGrid ;
 		std::string tracciato;
 		std::string asterismi;
 		std::string ecliptic;
     
		struct lookAt {
			
			std::string view = "undefined";
			std::string where = "earth";
		
		} lookat;

		struct Camera {
			
			double lookat_x, lookat_y, lookat_z;
			double up_x, up_y, up_z;
			double pos_x, pos_y, pos_z;
			
			double fov;
			double zoom;
			
			double near;
			double far;
			
			double old_fov = 0;
			double old_zoom = 0;
			double old_near = 0;
			double old_far = 0;
		
		} camera ;
		
		bool orbitControls = false;
		bool ambient_light = false;
		
		double FACT ;

		struct lookFrom {
			
			double latitude = 0;
			double longitude = 0;
			double move_lat;
			double move_lng;
			
			double dstOffset = 0;
			
			std::string where = "undefined";;
			bool isFromPlace = false;
		
		} lookfrom;
		
		clientData (  WSsrv * p, SolarSystem * _system, int socked_id ) :
			parent (p),
			system (_system),
			sockfd(socked_id),
			skyGrid( this )
			{
				
				timer.stop ();
				timer_result.stop ();
				
				set_default();
			
			}
		

		~clientData () {
		
			delete system ;
		
		}
			
		void set_default () {
			
			refresh = DEF_REFRESH ;	// secondi;
			scale = DEF_SCALE;
			simulation = DEF_SIMULATION ;
		
			lookfrom.isFromPlace = false;
			lookfrom.move_lat = DEF_MOVE_LAT;
			lookfrom.move_lng = DEF_MOVE_LNG;
			
			camera.up_x = DEF_UP_X;
			camera.up_y = DEF_UP_Y;
			camera.up_z = DEF_UP_Z;
			
			camera.fov = DEF_FOV;
			camera.zoom = DEF_ZOOM;
			
			camera.near = DEF_NEAR;
			camera.far = DEF_FAR;
			
			FACT = DEF_FACT;
		
		}
		
	
} ;
	

#endif	// if _wssrv_
