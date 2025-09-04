#include <math.h>       /* pow */
#include <sstream>
#include <fstream>
#include <exception>      // std::exception
#include <unistd.h>			// usleep

#include "WSsrv.hpp"
#include "angolo.hpp"
#include "Utils.h"

#include <line.hpp>

#ifndef __xml__
	#include <xml.hpp>
#endif

#include "clientHttp.hpp"


WSsrv::WSsrv ( unsigned int port, std::string & web_dir ):
    WebServerWS ( port, web_dir ),
	geoLoc ( "api.geonames.org" )
{
//	earthGrid.createTable();
    moonGrid = new Map("moon/moon_crater_1");
    earthGrid = new Map("earth/main_cities");

}

WSsrv::~WSsrv ( )

{
	delete moonGrid;
	delete earthGrid;
}


void WSsrv::initDb (  )
{
}

void WSsrv::handleClient (  int sockfd, std::string buffer )
{
	XMLDocument doc;
					
	std::string azione ;
	std::string command ;
	try {
	
		doc.parse( buffer );
		XMLElement * root = doc.rootElement ()  ; 
							
		azione = root->getAttribute ("azione");
		command = root->getAttribute ("data");
	
	}
	catch ( ... ) {
							
		azione = "command";
		command = buffer;
	
	}



#ifdef DEBUG
{
		std::stringstream cc ;
		cc << "handle request " << buffer;

		if ( azione != "reset_timer" )
			print_log( cc.str() );

}
#endif

	// cerca nei clients
	auto it =  clients.find( sockfd ) ;
	if ( it == clients.end() ) return;
	
	Client *  client  	= it->second;
	clientData * data = reinterpret_cast < clientData * > ( client->extra ); 
	short simulation = data->simulation;
	Date &d = data->ref_date;
	
	if ( !data ) return ;

//	SolarSystem * system = data->system;
	
	if ( azione == "command" ) {

		std::string result;
	
		std::stringstream cc ;
		cc.precision(16);		
		
		Date o;
		cc << command << " ";

		std::string action ;
		cc >> action;

		std::stringstream webgl ;

		if ( action == "mouse" || action == "mouse_move" || action == "mouse_dblclick" ) {

			UT uut = d;
			DT ddt = uut;
			double JD = uut.now( simulation );
//			double JD = d.now( simulation );

			std::string name;
			short button ;
			double x, y, z, distance ;
			
			cc >> button >> name >> x >> y >> z >> distance;
		
			std::stringstream term ;
			term << "<terminal>" << name << std::endl ;
			std::stringstream dlg ;
			dlg << "<dialog body=\"" << name << "\">" ;
			std::stringstream dt ;
			dt << "<data>" ;
			
			
			std::stringstream ss ;
			ss.precision( 12 );
			ss << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" ; 
			ss << "<results>" ;
				
			if ( name == "earth" ) {
				
				double r_xy = sqrt ( pow( x, 2 ) + pow( y, 2 ) );
				Grade lat = Radiant( atan( z / r_xy ) );



				// determina il quadrante
				bool sign_y = fabs(y) / y > 0 ? true : false;
				bool sign_x = fabs(x) / x > 0 ? true : false;
				short quadrante = 0;
	
				if ( sign_x && sign_y  ) quadrante = 1;
				else if ( !sign_x && sign_y ) quadrante = 2;
				else if ( !sign_x && !sign_y ) quadrante = 3;
				else if ( sign_x && !sign_y ) quadrante = 4;
				
				Grade lng  = Radiant( atan( fabs(y) / fabs(x) ) );
	
				switch ( quadrante ) {
					case 2 : lng = 180 - lng ;
					break;
					case 3 : lng = 180 + lng ;
					break;
					case 4 : lng = 360 - lng ;
					break;
					default :
					break;
				}


				lng = lng - data->rotation;
				std::stringstream tip = earthGrid->object( lng, lat ) ;
   
				
if ( button == 2 ) {
	
				try {
					
				
//http://api.geonames.org/cities?north=44.1&south=-9.9&east=-22.4&west=55.2&username=demo
					double north = lat + 1;
					double south = lat - 1;
					double east = lng + 1;
					double west = lng - 1;
					
					std::stringstream query ;
					query << "north=" << north
						<< "&south=" << south
						<< "&east=" << east
						<< "&west=" << west 
						<< "&username=baldoweb" ;
					
					TCPClient geoLoc ( "api.geonames.org" ) ;
					std::string geo_result = geoLoc.get( "/cities", query.str() );
//					print_log( geo_result );

					XMLDocument doc;
					doc.parse( geo_result );
					XMLElement * root = doc.rootElement ()  ; 
			
					auto geoname =  root->getElementsByTagName ( "geoname" ) ;
					for ( size_t i = 0; i < geoname.size(); ++i ) {
						
						XMLElement *geo = geoname.at(i);
						XMLElement * e_name =  geo->getFirstElementByTagName ( "name" ) ;
						XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
						XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
						XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

						std::string countryName, name, lat, lng;
						if ( e_countryName ) 
							countryName = e_countryName->innerHTML();
						if ( e_name  ) 
							name = e_name->innerHTML();
						if ( e_lat  ) 
							lat = e_lat->innerHTML();
						if ( e_lng  ) 
							lng = e_lng->innerHTML();

						dlg << "<form style=\"text-align:center;margin:1px\"" 
								<< " onsubmit=\"" 
								<< "exe_command ( 'zoom 1' );"
								<< "exe_command ( 'observe " << lng << " " << lat << " zenit' );"
								<< "return false;\">" 
								<< "<input type=\"submit\" value=\"" << name
								<< ", " << countryName 
								<< "\" /></form>"  ;
						

					}

				} catch ( const char * str ) {
					print_log( str );
				} catch ( const std::string & str ) {
					print_log( str );
				}	
				catch ( ... ) {
					print_log( "***" );
				}
				
}
				
				dlg << "<p>Latitudine = " << lat  << "</p><p>Longitudine = " << lng << "</p>"  ;
				term << "x=" << x << " y=" << y << " z=" << z << std::endl;   
				term << "Latitudine = " << lat << " Longitudine = " << lng << std::endl;  

				if ( data->camera.zoom < 2 ) {
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"\" />";
				}
				else if ( tip.str() == "" ) {
					tip << "Long " << lng << " Lat " << lat   ;
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}
				else {
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}

				if ( action == "mouse_dblclick" ) {
			

					data->lookfrom.isFromPlace = true;
					data->lookfrom.latitude = lat;
					data->lookfrom.longitude = lng;
					data->lookfrom.where = "earth";
					
					data->lookat.where = "zenit";
				
				}
			
			}
			else if ( name == "sky" ) {
				
				
				OrbitXYZ o_xyz ( x, y, z ) ;
				OrbitEqu o_eq = o_xyz ;
				
				float Dec = o_eq.delta ;
				float AR = o_eq.alfa ;	// gradi
				
				
				float ar = AR;
				
 				if ( AR < 0 )
					AR = 360 + AR  ;
 				
				
  
				SkyGrid & skyGrid = data->skyGrid;
				std::string const_name = skyGrid.whichConstellation( AR, Dec );
				std::stringstream zodiaco ;


				if ( fabs(Dec) < 30 ) {

					zodiaco <<" [" << skyGrid.whichZodiac ( AR ) << "]";

				}

				std::stringstream tip ;	
 				AR = 24 * AR / 360;
				tip << skyGrid.find( AR, Dec );	

				term <<  
					"Dec = " << Dec  << " (" << Grade(Dec).toGradeGPS() << ")" << std::endl <<  
						"AR = " << AR << " (" << Grade(ar).toGradeHMS() << ")";    
				
				dlg <<  
					"<p>Dec = " << Dec  << " (" << Grade(Dec).toGradeGPS() << ")</p>" <<  
					"<p>AR = " << AR << " (" << Grade(ar).toGradeHMS() << ")</p>" 	;   

				if ( tip.str() != "" ) {
					dt << "<div id=\"tip1\" value=\"" << tip.str() << "\" />";
					dt << "<div id=\"tip\" value=\"AR=" <<
						AR << " Dec=" << Dec << "\" />";
				}
				else {
					dt << "<div id=\"tip1\" value=\"" << "sky " << const_name << 
						zodiaco.str() <<  "\" />";
					dt << "<div id=\"tip\" value=\"AR=" <<
						AR << " Dec=" << Dec << "\" />";
				}
					
				if ( action == "mouse_dblclick" ) {
			
					data->lookfrom.isFromPlace = true;
					data->lookfrom.latitude = Dec;
					data->lookfrom.longitude = ar;
					
					data->lookat.where = "zenit";
				
				}
			
			
			}
			else if ( name == "sun" ) {

				Sun sun;
				OrbitLBR orbit_sun = sun.orbitLBR( JD );	// coordinate eclittiche


				OrbitEqu   o_sun ( JD );
				o_sun = orbit_sun;	// converte in coordinate equatoriali
				double ar = o_sun.alfa  ;
				double dec = o_sun.delta;
				
 				if ( ar < 0 )
					ar = 360 + ar  ;
		
				SkyGrid & skyGrid = data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;
				std::string zodiaco = skyGrid.whichZodiac ( ar ) ;

				Grade L = orbit_sun.L ;
				Grade B = orbit_sun.B ;
				Grade alfa = o_sun.alfa;
				Grade delta = o_sun.delta; // declinazione
		
				OrbitXYZ xyz_sun = o_sun ;	// converte da coordinate equatoriali in rettangolari
		
				double x_sun = xyz_sun.x ;
				double y_sun = xyz_sun.y ;
				double z_sun = xyz_sun.z ;

		// calcola direttamente con vsop le coordinate rettangolari
		OrbitXYZ orbit_sun_xyz = sun.orbit ( MainBody::earth_data, JD ) ;
		
		double x_s = -orbit_sun_xyz.x  ;
		double y_s = -orbit_sun_xyz.y ;
		double z_s = -orbit_sun_xyz.z  ;
		
		Earth earth;
		double epsilon = earth.obliquity ( JD );
		Radiant eps = Grade( epsilon );
		
		double x_eq = x_s;
		double y_eq = y_s * cos( eps ) - z_s * sin( eps );
		double z_eq = y_s * sin( eps ) + z_s * cos( eps );

				
				term << 
					"x=" << x_sun << " y=" << y_sun <<  " z=" << z_sun << std::endl <<  
					"x=" << x_eq << " y=" << y_eq <<  " z=" << z_eq << std::endl <<  
					"x=" << x_s << " y=" << y_s <<  " z=" << z_s << std::endl <<  
					"L = " <<  L <<" (" << L.toGradeGPS() << ")" << std::endl <<  
					"B = " << B << " (" << B.toGradeGPS() << ")" << std::endl <<  
					"AR = " << alfa << " (" << alfa.toGradeHMS() << ")" << std::endl <<  
					"Dec = " <<  delta << " (" << delta.toGradeGPS() << ")" << std::endl <<  
					"R = " << orbit_sun.R  ;
				
				dlg << 
					"<p>x=" << x_sun << " y=" << y_sun <<  " z=" << z_sun << "</p>" <<  
					"<p>x=" << x_eq << " y=" << y_eq <<  " z=" << z_eq << "</p>" <<  
					"<p>x=" << x_s << " y=" << y_s <<  " z=" << z_s << "</p>" <<  
					"<p>L = " <<  L <<" (" << L.toGradeGPS() << ")</p>" <<  
					"<p>B = " << B << " (" << B.toGradeGPS() << ")</p>" <<  
					"<p>AR = " << alfa << " (" << alfa.toGradeHMS() << ")</p>" <<  
					"<p>Dec = " <<  delta << " (" << delta.toGradeGPS() << ")</p>" <<  
					"<p>R = " << orbit_sun.R  << "</p>"  
				;
				dt << "<div id=\"tip1\" value=\"" << name << " in " << 
					constellation << " [" << zodiaco << "]\" />";
				dt << "<div id=\"tip\" value=\"\" />";

			}
			else if ( name == "moon" ) {

				
				Moon moon;

				OrbitAlfa orbit_moon = moon.orbitAlfaELP( JD );
				Grade lambda = orbit_moon.lambda;
				Grade beta = orbit_moon.beta;
    
				OrbitEqu   o_moon ( JD );
				o_moon = orbit_moon;
		
				double ar = o_moon.alfa  ;
				double dec = o_moon.delta;
				
 				if ( ar < 0 )
					ar = 360 + ar  ;
		
				SkyGrid & skyGrid = data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;

			
				OrbitXYZ o = o_moon;
				double x_moon = o.x;
				double y_moon = o.y;
				double z_moon = o.z;
				
				// traslazione
				x -= x_moon;
				y -= y_moon;
				z -= z_moon;
    
				double r_xy = sqrt ( pow( x, 2 ) + pow( y, 2 ) );
				Grade lat = Radiant( atan( z / r_xy ) );



				// determina il quadrante
				bool sign_y = fabs(y) / y > 0 ? true : false;
				bool sign_x = fabs(x) / x > 0 ? true : false;
				short quadrante = 0;
	
				if ( sign_x && sign_y  ) quadrante = 1;
				else if ( !sign_x && sign_y ) quadrante = 2;
				else if ( !sign_x && !sign_y ) quadrante = 3;
				else if ( sign_x && !sign_y ) quadrante = 4;
				
				Grade lng  = Radiant( atan( fabs(y) / fabs(x) ) );
		
				switch ( quadrante ) {
					case 2 : lng = 180 - lng ;
					break;
					case 3 : lng = 180 + lng ;
					break;
					case 4 : lng = 360 - lng ;
					break;
					default :
					break;
				}


				Grade rot_luna =  180 + o_moon.alfa;

				lng = lng - rot_luna;

				std::stringstream tip = moonGrid->object( lng, lat ) ;
				
				term << 
					"AR = " << o_moon.alfa << " (" << o_moon.alfa.toGradeHMS() << ")" << std::endl << 
					"Dec = " <<  o_moon.delta << " (" << o_moon.delta.toGradeGPS() << ")" << std::endl <<  
					"lambda = " <<  lambda << " (" << lambda.toGradeGPS() << ")" << std::endl << 
					"beta = " << beta << " (" << beta.toGradeGPS() << ")" << std::endl << 
					"R = " << orbit_moon.delta * CelestialBody::AU   << " km" << std::endl ;
				term << "Latitudine = " << lat << " Longitudine = " << lng ;   

				dlg <<
					"<p>Latitudine = " << lat << " Longitudine = " << lng << "</p>" <<std::endl << 
					"<p>AR = " << o_moon.alfa << " (" << o_moon.alfa.toGradeHMS() << ")</p>" <<  
					"<p>Dec = " <<  o_moon.delta << " (" << o_moon.delta.toGradeGPS() << ")</p>" <<  
					"<p>lambda = " <<  lambda << " (" << lambda.toGradeGPS() << ")</p>" <<  
					"<p>beta = " << beta << " (" << beta.toGradeGPS() << ")</p>" <<  
					"<p>R = " << orbit_moon.delta * CelestialBody::AU   << " km" << "</p>"  
				;
				
				if ( data->camera.zoom < 2 ) {
					dt << "<div id=\"tip1\" value=\"" << name << " in " << constellation << "\" />";
					dt << "<div id=\"tip\" value=\"\" />";
				}
				else if ( tip.str() == "" ) {
					tip << " Long " << lng << " Lat " << lat   ;
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}
				else {
					dt << "<div id=\"tip1\" value=\"" << name << "\" />";
					dt << "<div id=\"tip\" value=\"" << tip.str() << "\" />";
				}

			}
			else if ( 
				name == "mercury" ||
				name == "venus" || 
				name == "mars" || 
				name == "jupiter" || 
				name == "saturn" ||
				name == "uranus" 
				) {
				
				const VSOP * vsop ;
				if ( name == "venus" )
					vsop = & MainBody::venus_data;
				else if ( name == "mars" )
					vsop = & MainBody::mars_data;
				else if ( name == "jupiter" )
					vsop = & MainBody::jupiter_data;
				else if ( name == "mercury" )
					vsop = & MainBody::mercury_data;
				else if ( name == "saturn" )
					vsop = & MainBody::saturn_data;
				else if ( name == "uranus" )
					vsop = & MainBody::uranus_data;

				MainBody celestialBody;
				OrbitXYZ o_v = celestialBody.orbit_eq ( * vsop, JD );

				double x = o_v.x ;
				double y = o_v.y ;
				double z = o_v.z ;
				
				OrbitXYZ o_xyz ( x, y, z ) ;
				OrbitEqu o_eq = o_xyz ;
				
				Grade Dec = o_eq.delta ;
				Grade AR = o_eq.alfa ;
				
				double ar = o_eq.alfa  ;
				double dec = o_eq.delta;
				
 				if ( ar < 0 )
					ar = 360 + ar  ;
		
				SkyGrid & skyGrid = data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;


				term << 
					name << " "	<< x << " " << y << " " << z << std::endl <<
					"Dec = " << Dec  << " (" << Dec.toGradeGPS() << ")" << std::endl <<  
						"AR = " << AR << " (" << AR.toGradeHMS() << ")";    
				
				dlg << 
					"<p>Dec = " << Dec  << " (" << Dec.toGradeGPS() << ")</p>" <<  
					"<p>AR = " << AR << " (" << AR.toGradeHMS() << ")</p>" 	<< 
					"<p>"	<< x << " " << y << " " << z << "</p>" <<
					"<p>"	<< "R=" <<  o_eq.R << "</p>" ;
				dt << "<div id=\"tip1\" value=\"" << name << " in " << constellation << "\" />";
				dt << "<div id=\"tip\" value=\"\" />";

			}
			
			term <<  "</terminal>" ;
			dlg << "</dialog>" ;
			dt << "</data>" ;
			
			
			if ( action == "mouse" ) {
			
				ss << term.str();
				switch ( button ) {
					case 0 :
					break;
					case 2 :
						ss << dlg.str();
					break;
				}
			}
			else {
				ss << dt.str();
			}
			

			ss << "</results>" ;
			sendRFC6455 ( sockfd, "text", ss.str(), false );

		}
		else if ( action == "sun_rise" ) {


			if ( data->lookfrom.isFromPlace ) {
			
				double lat = data->lookfrom.latitude ;
				double lng = data->lookfrom.longitude ;


				Earth earth;
				double jd = earth.sunRise( d, lat, lng, simulation);
				Date d1 = jd;
			
			auto it = data->system->bodies.find("sun");
			
			CelestialBody & body = it->second;
			OrbitEqu o = body.position;
			Grade ar = o.alfa  ;
			Grade dec = o.delta;
				
	
	Radiant lt = Grade( lat );
	Radiant dc = dec;
	double cosomega0 = 
		( sin( Radiant( Grade(-0.83) ) ) - sin( lt ) * sin( dc ) )
		/
		( cos( lt ) * cos ( dc ) ) ;
					
	Grade  omega = Radiant ( acos( cosomega0 ) );
	
	double h = ( Grade( omega ) - lng ) * 24 / 360 ;
	
	SideralTime st ( d );
	Grade a (st.teta0());
	

				std::stringstream xml;
				xml.precision(16);		
				xml << 
					"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
					"<results>" << 
						"<terminal>" << d1.toDate() << " (" << jd << ") " << h << " " << a.toGradeHMS() << " " << a << "</terminal>" <<
					"</results>"
				;
				sendRFC6455 ( sockfd, "text", xml.str(), false );
			
			}
		
		}
		else if ( action == "angle" ) {


			if ( data->lookfrom.isFromPlace ) {
			
				double lat = data->lookfrom.latitude ;
				double lng = data->lookfrom.longitude ;

				double JD = d.now( simulation );

				if ( !cc.eof() ) 
					cc >> JD;

				Earth earth;
				double angle = earth.sunAngleOverOrizont( JD, lat, lng ) ;

				std::stringstream xml;
				xml.precision(8);		
				xml << 
					"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
					"<results>" << 
						"<terminal>" << angle  << " at " << JD << "</terminal>" <<
					"</results>"
				;
				sendRFC6455 ( sockfd, "text", xml.str(), false );
			
			}
		
		}
		else if ( action == "rotation" ) {
			
			double rot;
			cc >> rot;
			double y = tan( rot * 3.14 / 180 );
			double x = tan( (90-rot) * 3.14 / 180 );
			clientData::Camera &camera = data->camera;
			
/*
			double x1 = camera.pos_x;
			double y1 = camera.pos_y;
			double z1 = camera.pos_z;
*/
			
			double x2 = camera.lookat_x + x;
			double y2 = camera.lookat_y + y;
			double z2 = camera.lookat_z;
			
//			double r = sqr( pow(x2-x1,2) + pow(y2-y1,2) )  ;
			camera.up_x = x2 ;
			camera.up_y = y2 ;
			camera.up_z = z2+1 ;
		
		}
		else if ( action == "move" ) {
			double lat, lng;
			cc >> lat;
			if ( !cc.eof() ) {
				data->lookfrom.move_lat += lat ;
				cc >> lng;
				if ( !cc.eof() ) {
					data->lookfrom.move_lng += lng ;
				}
			}
			
			std::stringstream xml;
		
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
				"<results>" << 
					"<terminal>" << 
						"shift lat " << data->lookfrom.move_lat <<
						" shift lng " << data->lookfrom.move_lng <<
					"</terminal>" <<
				"</results>" ; 


			
			sendRFC6455 ( sockfd, "text", xml.str(), false );
		
		}
		else if ( action == "observe" ) {
			
			
 			double longitudine, latitudine;
			cc >> longitudine >> latitudine ;

			if ( cc.eof() ) {

				if ( data->lookfrom.isFromPlace ) {
			
					latitudine = data->lookfrom.latitude ;
					longitudine = data->lookfrom.longitude ;

				}
					
			}
			else {
			
				std::string verso;
				cc >> verso;
				if ( cc.eof() ) {
					data->lookat.where = "sun";
				}
				else {
			
					data->lookat.where = verso;
			
				}

				data->lookfrom.where = "earth";
				data->lookfrom.isFromPlace = true;
				data->lookfrom.latitude = latitudine;
				data->lookfrom.longitude = longitudine;
			
			}

			std::stringstream the_data ;
			the_data <<   
				"<input id=\"latitudine\" value=\"" << latitudine << "\" />"  <<
				"<input id=\"longitudine\" value=\"" << longitudine << "\" />"  ;
					
			short y = d.year();
			short m = d.month() + 1;
			short g = d.day();
			std::stringstream query ;
			query << "lat=" << latitudine
						<< "&lng=" << longitudine
						<< "&username=baldoweb"
						<< "&date=" << y << "-" << m << "-" << g ;

			std::stringstream ss ;
/*
			TCPClient geoLoc ( "api.geonames.org" ) ;
			std::string geo_result = geoLoc.get( "/timezone", query.str() );

			ss << "thanks to geonames.org" << endl;

			XMLDocument doc;
			doc.parse( geo_result );
			XMLElement * root = doc.rootElement ()  ; 
			
			auto geoname =  root->getElementsByTagName ( "timezone" ) ;
			for ( size_t i = 0; i < geoname.size(); ++i ) {
						
				XMLElement *geo = geoname.at(i);
				XMLElement * e_timezoneId =  geo->getFirstElementByTagName ( "timezoneId" ) ;
				XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
				XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
				XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

				XMLElement * e_gmtOffset = geo->getFirstElementByTagName ( "gmtOffset" ) ;
				XMLElement * e_dstOffset = geo->getFirstElementByTagName ( "dstOffset" ) ;
				XMLElement * e_rawOffset = geo->getFirstElementByTagName ( "rawOffset" ) ;
				XMLElement * e_time = geo->getFirstElementByTagName ( "time" ) ;

				XMLElement * e_sunrise = geo->getFirstElementByTagName ( "sunrise" ) ;
				XMLElement * e_sunset = geo->getFirstElementByTagName ( "sunset" ) ;

				std::string timezoneId, countryName, lat,
					 lng, gmtOffset, time, sunrise, sunset ;

				if ( e_countryName ) 
					ss << "Country " << e_countryName->innerHTML();
				if ( e_timezoneId ) 
					ss << "Zone " << e_timezoneId->innerHTML();
				if ( e_lat ) 
					ss << "Lat " << e_lat->innerHTML();
				if ( e_lng ) 
					ss << "Lng " << e_lng->innerHTML();
				if ( e_gmtOffset ) {
					ss << "gmtOffset " << e_gmtOffset->innerHTML();
				}
				if ( e_dstOffset ) {
					ss << "dstOffset " << e_dstOffset->innerHTML();
				}
				if ( e_rawOffset ) {
					ss << "rawOffset " << e_rawOffset->innerHTML();
					data->lookfrom.dstOffset = atof( e_rawOffset->innerHTML().c_str() );
				}
				if ( e_time ) 
					ss << "Time " << e_time->innerHTML();
				if ( e_sunrise ) 
					ss << "Sunrise " << e_sunrise->innerHTML();
				if ( e_sunset ) 
					ss << "Sunset " << e_sunset->innerHTML();

				ss << std::endl; 
						

			}
			
*/
			std::stringstream xml;
		
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
				"<results>" << 
					"<terminal>" << ss.str() << "</terminal>" <<
					"<data>" << the_data.str() << "</data>" <<
				"</results>" ; 


			
			sendRFC6455 ( sockfd, "text", xml.str(), false );
		
		}
		else if ( action == "lookfrom" ) {
			
			std::string fromwhere;
			cc >> fromwhere;
			if ( cc.eof() ) {

				return;

			}

			data->lookfrom.where = fromwhere;
 			
 			double longitudine, latitudine;
			cc >> longitudine >> latitudine ;

			if ( cc.eof() ) {

				latitudine = data->lookfrom.latitude ;
				longitudine = data->lookfrom.longitude ;
					
			}
			else {
			
				std::string verso;
				cc >> verso;
				if ( cc.eof() ) {
					data->lookat.where = "sun";
				}
				else {
			
					data->lookat.where = verso;
			
				}

				data->lookfrom.latitude = latitudine;
				data->lookfrom.longitude = longitudine;
			
			}

			data->lookfrom.isFromPlace = true;
			std::stringstream the_data ;
			std::stringstream ss ;
			if ( fromwhere == "earth" ) {
				
				the_data <<   
					"<input id=\"latitudine\" value=\"" << latitudine << "\" />"  <<
					"<input id=\"longitudine\" value=\"" << longitudine << "\" />"  ;
					
				short y = d.year();
				short m = d.month() + 1;
				short g = d.day();
				std::stringstream query ;
				query << "lat=" << latitudine
						<< "&lng=" << longitudine
						<< "&username=baldoweb"
						<< "&date=" << y << "-" << m << "-" << g ;

				TCPClient geoLoc ( "api.geonames.org" ) ;
				std::string geo_result = geoLoc.get( "/timezone", query.str() );

				ss << "thanks to geonames.org" << endl;

				XMLDocument doc;
				doc.parse( geo_result );
				XMLElement * root = doc.rootElement ()  ; 
			
				auto geoname =  root->getElementsByTagName ( "timezone" ) ;
				for ( size_t i = 0; i < geoname.size(); ++i ) {
						
					XMLElement *geo = geoname.at(i);
					XMLElement * e_timezoneId =  geo->getFirstElementByTagName ( "timezoneId" ) ;
					XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
					XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
					XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

					XMLElement * e_gmtOffset = geo->getFirstElementByTagName ( "gmtOffset" ) ;
					XMLElement * e_dstOffset = geo->getFirstElementByTagName ( "dstOffset" ) ;
					XMLElement * e_rawOffset = geo->getFirstElementByTagName ( "rawOffset" ) ;
					XMLElement * e_time = geo->getFirstElementByTagName ( "time" ) ;

					XMLElement * e_sunrise = geo->getFirstElementByTagName ( "sunrise" ) ;
					XMLElement * e_sunset = geo->getFirstElementByTagName ( "sunset" ) ;

					std::string timezoneId, countryName, lat,
						lng, gmtOffset, time, sunrise, sunset ;

					if ( e_countryName ) 
						ss << "Country " << e_countryName->innerHTML();
					if ( e_timezoneId ) 
						ss << "Zone " << e_timezoneId->innerHTML();
					if ( e_lat ) 
						ss << "Lat " << e_lat->innerHTML();
					if ( e_lng ) 
						ss << "Lng " << e_lng->innerHTML();
					if ( e_gmtOffset ) {
						ss << "gmtOffset " << e_gmtOffset->innerHTML();
					}
					if ( e_dstOffset ) {
						ss << "dstOffset " << e_dstOffset->innerHTML();
					}
					if ( e_rawOffset ) {
						ss << "rawOffset " << e_rawOffset->innerHTML();
						data->lookfrom.dstOffset = atof( e_rawOffset->innerHTML().c_str() );
					}
					if ( e_time ) 
						ss << "Time " << e_time->innerHTML();
					if ( e_sunrise ) 
						ss << "Sunrise " << e_sunrise->innerHTML();
					if ( e_sunset ) 
						ss << "Sunset " << e_sunset->innerHTML();
	
					ss << std::endl; 
						

				}
				
			}
			

			std::stringstream xml;
		
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
				"<results>" << 
					"<terminal>" << 
						"lookfrom " << data->lookfrom.where << " to " << data->lookat.where << std::endl <<
						"lat " << data->lookfrom.latitude << " lng " << data->lookfrom.longitude << std::endl << ss.str() << "</terminal>" <<
					"<data>" << the_data.str() << "</data>" <<
				"</results>" ; 


			
			sendRFC6455 ( sockfd, "text", xml.str(), false );
		
		}
		else if ( action == "sky" ) {

			Sky sky;
				
			double R = sky.radius;
			double ascensione_retta ;
			double declinazione ;
			
			cc >> ascensione_retta ;
			
			std::stringstream ss ;

			if ( cc.eof() ) {
				
				ss << "Please set R.A. (hours) and Dec (degree)" << std::endl;
				
			}				
			else {

				cc >> declinazione;

				if ( cc.eof() ) {
				ss << "Please set R.A. (hours) and Dec (degree)" << std::endl;
				}
				else {
			
					declinazione += data->lookfrom.move_lat;
					ascensione_retta += data->lookfrom.move_lng;
			
					Radiant a = Grade( ascensione_retta *  360 / 24  );
					Radiant d = Grade( declinazione  );
			
					double z = R * sin( d );
					double r = R * cos( d );

					double x = r * cos( a );
					double y = r * sin( a );


					clientData::Camera &camera = data->camera;
					camera.lookat_x = x ;
					camera.lookat_y = y ;
					camera.lookat_z = z ;
/*
					camera.pos_x = p.x;
					camera.pos_y = p.y;
					camera.pos_z = p.z;
*/				
					data->lookat.where = "sky";
		
					std::string AR = AscensioneRetta( a );
					std::string Dec = Declinazione( d );
			
					ss <<  
						"RA=" << AR << " Dec=" << Dec << " x=" << x <<
						" y=" << y <<
						" z=" << z  ;

				}
			
			}

			sendRFC6455 ( sockfd, "text", ss.str(), false );
		
		
		}
		else if ( action == "constellation" ) {

			Sky sky;
				
			std::string name, n1, n2;
			cc >> n1 >> n2;

			name = utils::trim(n1)  ;

			bool hidden = false;
			if ( utils::trim(n2) == "hidden" ) {
				hidden = true;
			}
			
			data->tracciato = data->skyGrid.drawBounds ( name, hidden );
/*			
			auto it = starGrid.find_costellation ( name );
			
			if ( it != starGrid.costellations.end() ) {
				
				SkyMap::Constellation * c = & it->second;
				
				data->tracciato = "";
				
				data->tracciato = c->drawBounds( hidden );
			}
			else {
				
				std::stringstream ss ;
				data->tracciato = "";
				for ( auto it = starGrid.costellations.begin(); it != starGrid.costellations.end(); ++it ) {
				
					SkyMap::Constellation * c = & it->second;
					data->tracciato += c->drawBounds( true );

				}
				

				ss <<  
					"Non è registrata alcuna costellazione " << name;

				sendRFC6455 ( sockfd, "text", ss.str(), false );
				
			}
*/
//			std::vector < short, short > v = c.tracciato;
			
		
		
		}
		else if ( action == "asterism" ) {

			Sky sky;
				
			std::string name, n1, n2;
			cc >> n1 >> n2;

			name = utils::trim (n1)  ;

			bool hidden = false;
			if ( utils::trim(n2) == "hidden" ) {
				hidden = true;
			}
			
			data->asterismi = "";
			data->asterismi = data->skyGrid.asterism ( name, hidden );
			sendRFC6455 ( sockfd, "text", data->asterismi, false );

		}
		else if ( action == "date" ) {
			
			std::stringstream ss ;
			std::string date;
			std::string time;
			cc >> date;
			
			if ( !cc.eof()  ) {
			
				cc >> time;
				if ( cc.eof() )
					time ="00:00:00";
			
				std::string ut;
				cc >> ut;
				if ( cc.eof() )
					ut = "UT";

				std::string dt = date + " " + time;
				if ( ut == "DT" ) {
					
					DT dtt = dt;
					UT utt = dtt;
					
					data->ref_date = utt;

				}
				else {

					data->ref_date = dt;

				}
				
		
			}
			else {
				ss << "Format dd-mm-yyy hh:mm:ss" << std::endl;
			}

			double jd = data->ref_date.now( simulation );
/*
			ss.precision(10);
			ss << "Date set to ";
			ss << data->ref_date.toDate(jd) << " jd " << jd <<  std::endl;

			sendRFC6455 ( sockfd, "text", ss.str(), false );
*/

			std::string dd = data->ref_date.toDate(jd);

			ss.precision(10);
			ss << "Date set to ";
			ss << dd << " jd " << jd <<  std::endl;

			std::stringstream the_data;
			
			{
			
				std::vector < std::string > v1;
				utils::split( dd, " ", v1 );
				std::string date = v1[0];
				std::string time = v1[1];
			
				std::vector < std::string > v2;
				utils::split( date, "-", v2 );
				std::string day = v2[0];
				if ( day.size() == 1 )
					day = "0" + day;
				std::string month = v2[1];
				if ( month.size() == 1 )
					month = "0" + month;
				std::string year = v2[2];
				if ( year.size() != 4 ) {
					year = "0000" + month;
					year = year.substr( year.size() - 4 );
				}

				std::vector < std::string > v3;
				utils::split( time, ":", v3 );
				std::string hours = v3[0];
				if ( hours.size() == 1 )
					hours = "0" + hours;
				std::string minutes = v3[1];
				if ( minutes.size() == 1 )
					minutes = "0" + minutes;
				std::string seconds = v3[2];
				if ( seconds.size() == 1 )
					seconds = "0" + seconds;

			
				the_data <<
					"<input type=\"text\" id=\"date\" value=\"" <<
					day << "-" << month << "-" << year << "\" />" <<
					"<input type=\"text\" id=\"time\" value=\"" << hours << ":" << minutes << "\" />" 
				;
		
			}

			std::stringstream xml;
		
			xml << 
				"<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
				"<results>" << 
					"<terminal>" << ss.str() << "</terminal>" <<
					"<data>" << the_data.str() << "</data>" <<
				"</results>"
			;
			sendRFC6455 ( sockfd, "text", xml.str(), false );
			

			return;

		}
		else if ( action == "scale" ) {
			
			if ( !cc.eof() ) {

				double value ;
				cc >> value;
				data->scale = value;

			}

			std::stringstream ss ;
			ss << "Scale value is set to ";
			ss << data->scale << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;

		}
		else if ( action == "refresh" ) {
			
			std::string refresh;
			cc >> refresh;
			
			if ( !cc.eof() ) {

				if ( utils::isNumber( refresh ) )
					data->refresh = atoi(refresh.c_str() );
					
			}
			
			std::stringstream ss ;
			ss << "Refresh set to ";
			ss << data->refresh << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "light" ) {
			
			std::string ambient;
			cc >> ambient;
			
			if ( !cc.eof() ) {

				if ( ambient == "true" )
					data->ambient_light = true;
				else
					data->ambient_light = false;
					
			}
			
			
			std::stringstream ss ;
			ss << "Ambient light set to ";
			ss << data->ambient_light << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );

			return;
		}
		else if ( action == "eclittica" ) {
			
			std::string value;
			cc >> value;
			
			if ( !cc.eof() ) {

				SkyGrid & skyGrid = data->skyGrid;
			
				if ( value == "false" )
					data->ecliptic = skyGrid.trackEcliptic(true);
				else
					data->ecliptic = skyGrid.trackEcliptic(false);

				std::stringstream ss ;
				ss << "Track ecliptic is set to ";
				ss << value << std::endl;
		
				sendRFC6455 ( sockfd, "text", ss.str(), false );
			
			}
			
			

			return;
		}
		else if ( action == "orbitControl" ) {
			
			std::string c;
			cc >> c;
			
			if ( c == "1" ) {
				data->orbitControls = true;
				data->refresh = 60;
			}
			else {
				data->orbitControls = false;
				data->refresh = 0;
			}
			
			std::stringstream ss ;
			ss << "Orbit controls set to ";
			ss << data->orbitControls << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );

			return;
		}
		else if ( action == "simulation" ) {
			
			std::string simulation;
			cc >> simulation;
			
			if ( utils::isNumber( simulation ) ) {
			
				JDay jd = data->ref_date.now( data->simulation );
			
				data->simulation = atoi( simulation.c_str() );
				data->ref_date = jd;
			
			}
			
			std::stringstream ss ;
			ss << "Simulation set to ";
			ss << data->simulation << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "default" ) {
			
			data->set_default ();
			
			std::stringstream ss ;
			ss << "Default values set";
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "fov" ) {
			double fov;
			cc >> fov;
			if ( !cc.eof() ) {
				data->camera.fov = fov;
			}
			
			std::stringstream ss ;
			ss << "Camera fov set to ";
			ss << data->camera.fov << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "zoom" ) {
			
			std::string zoom;
			cc >> zoom;
			
			if ( !cc.eof() ) {
				
				if ( utils::isNumber( zoom ) ) {
					
					double val = atof( zoom.c_str() );
					if ( val > 0 )
						data->camera.zoom = val;
				
				}
				else {
					
					if ( utils::trim( zoom ) == "+" ) {
						data->camera.zoom *= 2 ;
					}
					else if ( utils::trim(zoom) == "-" ) {
						data->camera.zoom /= 2;
					}
				
				}
				
			}
			
			std::stringstream ss ;
			ss << "Camera zoom set to ";
			ss << data->camera.zoom << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "near" ) {
			double near;
			cc >> near;
			if ( !cc.eof() ) {
				data->camera.near = near;
			}
			
			std::stringstream ss ;
			ss.precision(12);
			ss << "Camera near set to ";
			ss << data->camera.near << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "far" ) {
			double far;
			cc >> far;
			if ( !cc.eof() ) {
				data->camera.far = far;
			}
			
			std::stringstream ss ;
			ss << "Camera far set to ";
			ss << data->camera.far << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "factor" ) {
			double fact;
			cc >> fact;
			if ( !cc.eof() ) {
				data->FACT = fact;
			}
			
			std::stringstream ss ;
			ss << "Factor set to ";
			ss << data->FACT << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "tropical" || action == "tropic" ) {
			

			Date &d = data->ref_date;

			std::string a;
			cc >> a;

			if ( cc.eof() ) {
				return;
			}
			else if ( a != "year" ) {
				return;
			}

			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			Earth earth;
			double jd1 = earth.equinox(year, 3);
			double jd2 = earth.equinox(year-1,3);

			double tropic_year = jd1 - jd2;
					
			std::stringstream ss ;
			ss.precision(6);
			ss << "Tropical year " << year << " (vernal equinox) : " <<
				tropic_year << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;
		
		}
		else if ( action == "equinox" ) {
			

			Date &d = data->ref_date;
			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			std::stringstream ss ;
			Earth earth;
					

{

			try {
			
				double jm = earth.marchEquinox ( year );
				DT dm ( jm );
				UT utm ( dm );
			
				ss << "March equinox " <<" " <<
					"DT " << dm.toDate() <<" " <<
					"UT " << utm.toDate() << std::endl;
					
				double js = earth.septemberEquinox ( year );
				DT ds ( js );
				UT uts ( ds );
			
				ss << "September equinox " <<" " <<
					"DT " << ds.toDate() <<" " <<
					"UT " << uts.toDate() << std::endl;
			} catch ( const std::string & e ) {
				ss << e << std::endl;
			}
}
/*
{

			double jm = earth.equinox ( year, 3 );
			DT dm ( jm );
			UT utm ( dm );
			
			ss << "March equinox " << " " <<
				"DT " << dm.toDate() << " " <<
				"UT " << utm.toDate() << std::endl;
					
			double js = earth.equinox ( year, 9 );
			DT ds ( js );
			UT uts ( ds );
			
			ss << "September equinox " <<  " " <<
				"DT " << ds.toDate() << " " <<
				"UT " << uts.toDate() << std::endl;
}
*/
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );


/*
			Date &d = data->ref_date;
			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			std::stringstream ss ;
			Earth earth;
					
			double jm = earth.marchEquinox ( year );
			DT dm ( jm );
			
			ss << "March equinox " << std::endl;
			ss << "DT " << dm.toDate() << std::endl;
			UT utm ( dm );
			ss << "UT " << utm.toDate() << std::endl;
					
			double js = earth.septemberEquinox ( year );
			DT ds ( js );
			
			ss << "September equinox " << std::endl;
			ss << "DT " << ds.toDate() << std::endl;
			UT uts ( ds );
			ss << "UT " << uts.toDate() << std::endl;
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
*/
			return;
		
		}
		else if ( action == "solstice" ) {
			
			Date &d = data->ref_date;
			short year ;
			cc >> year ;

			if ( cc.eof() ) {
				year = d.year();
			}

			std::stringstream ss ;
			Earth earth;
					
			try {
				
				double j = earth.juneSolstice ( year );

				DT dt ( j );
				UT ut ( dt );
			
				ss << "June solstice " << " " <<
					"DT " << dt.toDate() << " " <<
					"UT " << ut.toDate() << std::endl;

			}
			catch ( const std::string & err) {
				ss << err << std::endl;
			}
			
			try {
				
				double j = earth.decemberSolstice ( year );

				DT dt ( j );
				UT ut ( dt );
			
				ss << "December solstice " << " " <<
					"DT " << dt.toDate() << " " <<
					"UT " << ut.toDate() << std::endl;

			}
			catch ( const std::string & err) {
				ss << err << endl;
			}
			catch ( ... ) {
			}
			
			sendRFC6455 ( sockfd, "text", ss.str(), false );

			return;
		
		}
		else if ( action == "up" ) {
			clientData::Camera &camera = data->camera;
			double x, y, z;
			cc >> x ;
			if ( !cc.eof() ) {
				cc >> y;
				if ( !cc.eof() ) {
					cc >> z;
					if ( !cc.eof() ) {
						camera.up_x = x ;
						camera.up_y = y ;
						camera.up_z = z ;
					}
				}
			}
			std::stringstream ss ;
			ss << "up " << camera.up_x <<
				" " << camera.up_y <<
				" " << camera.up_z;
			
			sendRFC6455 ( sockfd, "text", ss.str(), false );

		}
		else if ( action == "lookat" ) {
			
			std::stringstream result;
			result.precision(12);
		
		
			std::string lookat;
			cc >> lookat;
			if ( lookat == "earth" ) {
				if ( !cc.eof() ) {
					
					std::string vista;
					cc >> vista;

					if ( vista == "moon" )
						data->lookat.where = "earth moon";
					else
						data->lookat.where = "earth";
				
				}
				else
					data->lookat.where = "earth";
			}
			else if ( lookat == "moon" ) {

				data->lookat.where = "moon";

				
/*
				Moon moon;
				OrbitAlfa orbit = moon.orbitAlfaELP( JD );
				Grade alfa = orbit.lambda;
				Grade delta = orbit.beta; // declinazione
				double R = orbit.delta;
				result << "alfa =" << alfa <<
					" delta=" << delta <<
					" R=" << R << std::endl;
*/
			}
			else if ( lookat == "sun" ) {
				
		
				UT uut = d;
				DT ddt = uut;
				double JD = uut.now( simulation );

		Sun sun;

		OrbitLBR orbit_sun = sun.orbitLBR( JD );	// coordinate eclittiche
		
		OrbitEqu   o_sun ( JD );
		// converte le coordinate eclittiche in equatoriali
		o_sun = orbit_sun;	
		
		Grade alfa_sun = o_sun.alfa;	// AR
		Grade delta_sun = o_sun.delta; // declinazione
		
		
		OrbitXYZ o_xyz = o_sun;	// converte in coordinate rettangolari
		double x_sun = o_xyz.x ;
		double y_sun = o_xyz.y ;
		double z_sun = o_xyz.z ;
		
				Point p1 (0, 0, 0);
				Point p2 ( x_sun, y_sun, z_sun );
			
				double r = p1.distance( p2 );
			
				StraightLine line ( p2, p1);

				Point p = line.distanceToPoint( r * 0.999 );

				clientData::Camera &camera = data->camera;

				camera.pos_x = p.x ;
				camera.pos_y = p.y ;
				camera.pos_z = p.z ;
				
				data->lookat.where = "sun";
			
			}
			else if ( lookat == "sky" )
				data->lookat.where = "sky";
			else {
				data->lookat.where = lookat;
			}

			std::stringstream ss ;
			ss << "Look at ";
			ss << data->lookat.where << std::endl;
			ss << result.str();
		
			sendRFC6455 ( sockfd, "text", ss.str(), false );
			return;

		}
		else if ( action == "view" ) {
			
			std::string direction;
			cc >> direction;
			if ( !cc.eof() ) {
					
				if ( data->lookfrom.where == "earth" ) {
			
					double latitude = data->lookfrom.latitude;
					double longitude = data->lookfrom.longitude;
			
					double rot = data->rotation ;
					Earth & earth = data->system->earth ;
					Point coord = earth.bodyCoord ( latitude, longitude + rot );

					double xc = coord.x / CelestialBody::AU;
					double yc = coord.y / CelestialBody::AU;
					double zc = coord.z / CelestialBody::AU;
				
					// intersezione piano tangente a coord e asse z
					double z = ( xc * xc + yc * yc + zc * zc ) / zc;
			
					// retta ortogonale al punto di vista
					StraightLine pdv ( Point( 0, 0, 0 ), Point( xc, yc, zc ) );
					
					if ( direction == "N" ) {
						
						clientData::Camera &camera = data->camera;
			
						data->lookat.where = "sky";
						data->lookat.view = "N";
						
						camera.lookat_x = 0;
						camera.lookat_y = 0;
						camera.lookat_z = z  ;
					
					}
					else if ( direction == "S" ) {
						
						
						Point p1 ( xc, yc, zc );
						Point p2 ( 0, 0, z  );
			
						double r = p1.distance( p2 );
			
						StraightLine line ( p2, p1 );

						Point p = line.distanceToPoint( 10 * r );
						
						clientData::Camera &camera = data->camera;
			
						data->lookat.where = "sky";
						data->lookat.view = "S";
						
						camera.lookat_x = p.x;
						camera.lookat_y = p.y;
						camera.lookat_z = p.z;
						
					}
					else if ( direction == "E" ) {
						
						// intersezione piano tangente a coord e asse z
						StraightLine int_z (  Point( xc, yc, zc ), Point( 0, 0, z) );
			
						Direction D = pdv.orto( int_z );					
						StraightLine orto ( D, Point( xc, yc,zc ) ) ;
						
						clientData::Camera &camera = data->camera;
			
						data->lookat.where = "sky";
						data->lookat.view = "E";
						
						Point p = orto.distanceToPoint ( 1);
						camera.lookat_x = p.x ;
						camera.lookat_y = p.y ;
						camera.lookat_z = p.z ;
						
					}
					else if ( direction == "W" ) {
						
						// intersezione piano tangente a coord e asse z
						StraightLine int_z (  Point( 0, 0, z), Point( xc, yc, zc ) );
			
						Direction D = pdv.orto( int_z );					
						StraightLine orto ( D, Point( xc, yc,zc ) ) ;
						
						clientData::Camera &camera = data->camera;
			
						data->lookat.where = "sky";
						data->lookat.view = "W";
						
						Point p = orto.distanceToPoint ( 1 );
						camera.lookat_x = p.x ;
						camera.lookat_y = p.y ;
						camera.lookat_z = p.z ;
						
						
					}
					else if ( direction == "Z" ) {
						
						clientData::Camera &camera = data->camera;
			
						data->lookat.where = "sky";
						data->lookat.view = "Z";
						
						Point p ( 10 * xc, 10 + yc, 10 * zc );
						camera.lookat_x = p.x ;
						camera.lookat_y = p.y ;
						camera.lookat_z = p.z ;
						
					}
		
		
				}
				
			}

		}
		else {
		
			std::stringstream result;
			result.precision(12);
		
			clientData::Camera &camera = data->camera;
			auto it = data->system->bodies.find( action );
			if ( it != data->system->bodies.end() ) {
			
				CelestialBody & body = it->second;
				OrbitEqu o = body.position;
				double ar = o.alfa  ;
				double dec = o.delta;
				
 				if ( ar < 0 )
					ar = 360 + ar  ;

				SkyGrid & skyGrid = data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;
				std::string zodiaco = skyGrid.whichZodiac ( ar );

				result << it->first << " in " << constellation <<  
					" [zodiac: " << zodiaco << "]" << std::endl <<
					"\tDec=" << o.delta.toGradeGPS() << " AR=" << o.alfa.toGradeHMS() << " R=" << o.R << std::endl <<
					"\tDec=" << dec << " AR=" << ar << " R=" << o.R << std::endl ;
			
			}
			else {
				
				UT ut = d.now( simulation );
				Date LT ( d.now( simulation ) + data->lookfrom.dstOffset / 24 );
				DT dt = ut ;
			
				std::stringstream cities;
				std::stringstream ss;
				if ( data->lookfrom.where == "earth" ) {

					double north = data->lookfrom.latitude + 0.05;
					double south = data->lookfrom.latitude - 0.05;
					double east = data->lookfrom.longitude + 0.05;
					double west = data->lookfrom.longitude - 0.05;
					
					std::stringstream query ;
					query << "north=" << north
						<< "&south=" << south
						<< "&east=" << east
						<< "&west=" << west 
						<< "&username=baldoweb" ;
					
					TCPClient geoLoc ( "api.geonames.org" ) ;
					std::string geo_result = geoLoc.get( "/cities", query.str() );
					
					XMLDocument doc;
					doc.parse( geo_result );
					XMLElement * root = doc.rootElement ()  ; 
			
					auto geoname =  root->getElementsByTagName ( "geoname" ) ;
					for ( size_t i = 0; i < geoname.size(); ++i ) {
						
						XMLElement *geo = geoname.at(i);
						XMLElement * e_name =  geo->getFirstElementByTagName ( "name" ) ;
						XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
						XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
						XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

						std::string countryName, name, lat, lng;
						if ( e_countryName ) 
							countryName = e_countryName->innerHTML();
						if ( e_name  ) 
							name = e_name->innerHTML();
						if ( e_lat  ) 
							lat = e_lat->innerHTML();
						if ( e_lng  ) 
							lng = e_lng->innerHTML();

						cities  << name.replace(name.end()-1, name.end(), "")
								<< ", " << countryName.replace(countryName.end()-1, countryName.end(),""); 
								
						break;

					}
					

					{

						short y = d.year();
						short m = d.month() + 1;
						short g = d.day();
						std::stringstream query ;
						query << "lat=" << data->lookfrom.latitude
								<< "&lng=" << data->lookfrom.longitude
								<< "&username=baldoweb"
								<< "&date=" << y << "-" << m << "-" << g ;

						TCPClient geoLoc ( "api.geonames.org" ) ;
						std::string geo_result = geoLoc.get( "/timezone", query.str() );

						ss << "thanks to geonames.org" << endl;

						XMLDocument doc;
						doc.parse( geo_result );
						XMLElement * root = doc.rootElement ()  ; 
			
						auto geoname =  root->getElementsByTagName ( "timezone" ) ;
						for ( size_t i = 0; i < geoname.size(); ++i ) {
						
							XMLElement *geo = geoname.at(i);
							XMLElement * e_timezoneId =  geo->getFirstElementByTagName ( "timezoneId" ) ;
							XMLElement * e_countryName = geo->getFirstElementByTagName ( "countryName" ) ;
							XMLElement * e_lat = geo->getFirstElementByTagName ( "lat" ) ;
							XMLElement * e_lng = geo->getFirstElementByTagName ( "lng" ) ;

							XMLElement * e_gmtOffset = geo->getFirstElementByTagName ( "gmtOffset" ) ;
							XMLElement * e_dstOffset = geo->getFirstElementByTagName ( "dstOffset" ) ;
							XMLElement * e_rawOffset = geo->getFirstElementByTagName ( "rawOffset" ) ;
							XMLElement * e_time = geo->getFirstElementByTagName ( "time" ) ;

							XMLElement * e_sunrise = geo->getFirstElementByTagName ( "sunrise" ) ;
							XMLElement * e_sunset = geo->getFirstElementByTagName ( "sunset" ) ;

							std::string timezoneId, countryName, lat,
								lng, gmtOffset, time, sunrise, sunset ;

							if ( e_countryName ) 
								ss << "\tCountry " << e_countryName->innerHTML();
							if ( e_timezoneId ) 
								ss << "\tZone " << e_timezoneId->innerHTML();
							if ( e_lat ) 
								ss << "\tLat " << e_lat->innerHTML();
							if ( e_lng ) 
								ss << "\tLng " << e_lng->innerHTML();
							if ( e_gmtOffset ) {
								ss << "\tgmtOffset " << e_gmtOffset->innerHTML();
							}
							if ( e_dstOffset ) {
								ss << "\tdstOffset " << e_dstOffset->innerHTML();
							}
							if ( e_rawOffset ) {
								ss << "\trawOffset " << e_rawOffset->innerHTML();
								data->lookfrom.dstOffset = atof( e_rawOffset->innerHTML().c_str() );
							}
							if ( e_time ) 
								ss << "\tTime " << e_time->innerHTML();
							if ( e_sunrise ) 
								ss << "\tSunrise " << e_sunrise->innerHTML();
							if ( e_sunset ) 
								ss << "\tSunset " << e_sunset->innerHTML();
	
							ss << std::endl; 
						

						}

					}
				
				}
				
				result << 
					"UT " << ut.toDate() <<  std::endl <<
					"LT " << LT.toDate() << " (add DST if needed) " <<
					"DT " << dt.toDate()  << " " <<
					"JD " << dt.julianDay() << std::endl <<
					"look from " << data->lookfrom.where << std::endl <<
					"\t" << cities.str() << std::endl << 
					"\t" << ss.str() << std::endl << 
					"\tx=" << camera.pos_x << " y=" << camera.pos_y << " z=" << camera.pos_z << std::endl <<
					"look to " << data->lookat.where << std::endl <<
					"\tx=" << camera.lookat_x << " y=" << camera.lookat_y << " z=" << camera.lookat_z << std::endl ;
				
			}
			
			std::stringstream xml;
		
			xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
					"<results>" << 
						"<terminal>" << 
						result.str() <<
						"</terminal>" <<
					"</results>" ; 


			
			sendRFC6455 ( sockfd, "text", xml.str(), false );

		}

	}


}


void WSsrv::do_before_sendLoop ( Client * /* client */ ) 
{
	

}

void WSsrv::do_sendLoop ( Client *  client  )
{
								
	int sockfd = client->socket ;

	clientData * data = reinterpret_cast < clientData * > ( client->extra ); 
	
	if ( !data ) return ;

	// scandisce il refresh dei dati grafici a monitor
	Timer & timer = data->timer;
	const auto elapsed = timer.time_elapsed();
	milliseconds msecs = duration_cast<milliseconds>(elapsed);

	// scandisce la scrittura dei dati a terminale
	Timer & timer_result = data->timer_result;
	const auto elapsed_res = timer_result.time_elapsed();
	milliseconds msecs_res = duration_cast<milliseconds>(elapsed_res);

	// accelera la simulazione di data->simulation secondi
	short simulation = data->simulation;
	// la data di riferimento
	Date &d = data->ref_date;
	// giorno giuliano attuale, la data + tempo passato dallo start
	// aggiustato con il tempo di simulazione
	UT uut = d;
	DT ddt = uut;
	double JD = uut.now( simulation );
	
	if ( !timer.isRunning() || msecs > milliseconds( 1000 * data->refresh ) ) {
		
	
		// resetta il timer
		timer.start();



		Sun & sun = data->system->sun ;

		OrbitLBR orbit_sun = sun.orbitLBR( JD );
		
		OrbitEqu   o_sun ( JD );
		// converte le coordinate eclittiche in equatoriali
		o_sun = orbit_sun;
		
		Grade alfa_sun = o_sun.alfa;	// AR
		Grade delta_sun = o_sun.delta; // declinazione
		
		// converte in coordinate rettangolari
		OrbitXYZ o_sun_xyz = o_sun;
		double x_sun = o_sun_xyz.x ;
		double y_sun = o_sun_xyz.y ;
		double z_sun = o_sun_xyz.z ;
		
		sun.position = o_sun_xyz ;
/*

				Sun sun;
				double r_sun = sun.radius / CelestialBody::AU ;
				OrbitLBR orbit_sun = sun.orbitLBR( JD );	// coordinate eclittiche

            double L1 = orbit_sun.L;
            double R = orbit_sun.R;
				
            double FK5 = - 0.09033 / 3600;
            double aberrazione = - ( 20.4898 / R ) / 3600;
 
            Earth earth1;
            Earth::Nutation n = earth1.nutation( JD );
            // facciamo il complemento
            if ( L1 < 0 ) L1 += 360;
			orbit_sun.L = L1 + n.Dpsi + FK5 + aberrazione;;

				OrbitEqu   o_sun ( JD );

				o_sun = orbit_sun;	// converte in coordinate equatoriali
		
				Grade L = orbit_sun.L ;
				Grade B = orbit_sun.B ;
				Grade alfa = o_sun.alfa;
				Grade delta = o_sun.delta; // declinazione
		
				OrbitXYZ xyz_sun = o_sun ;	// converte da coordinate equatoriali in rettangolari
		
				double x_sun = xyz_sun.x ;
				double y_sun = xyz_sun.y ;
				double z_sun = xyz_sun.z ;
		
*/	
		Earth & earth = data->system->earth ;
		double r_earth = earth.radius / CelestialBody::AU ;
		double rot = earth.rotation ( ddt, data->simulation ) ;
		data->rotation = rot;
		OrbitXYZ orbit_earth_xyz ( 0, 0, 0 ) ;
		orbit_earth_xyz.rot_y = rot ;
		Grade epsilon = earth.obliquity ( JD );
		earth.obliq = epsilon;

		earth.position = orbit_earth_xyz ;
		
		Moon & moon = data->system->moon ;
		double r_moon = moon.radius / CelestialBody::AU ;

		// i dati della teoria ELP
		OrbitAlfa orbit_moon = moon.orbitAlfaELP( JD );
		Grade lambda_moon = orbit_moon.lambda;
		Grade beta_moon = orbit_moon.beta;
		
		OrbitEqu   o_moon ( JD );
		// converte le coordinate eclittiche in equatoriali
		o_moon = orbit_moon;
		
		Grade alfa_moon = o_moon.alfa;
		Grade delta_moon = o_moon.delta; // declinazione
		
		
		// converte in coordinate rettangolari
		OrbitXYZ o_moon_xyz = o_moon;
		double x_moon = o_moon_xyz.x ;
		double y_moon = o_moon_xyz.y ;
		double z_moon = o_moon_xyz.z ;
		o_moon_xyz.rot_x += 1.543 ;
    
		moon.position = o_moon_xyz ;
		// orientamento della luna rispetto alla terra
		// devo ruotare esattamente di alfa_moon
		//		Grade rot_luna = orbit_moon.lambda;
		// TODO non funziona ancora
		Grade rot_luna =  180 + alfa_moon;
		o_moon_xyz.rot_y = rot_luna ;
		
		/*
		l'orbita dei pianeti è calcolata con VSOP C (rettangolare ecclittica eliocentrica)
		* convertita ad equatoriale geocentrica
		utilizzando l'orbita rettangolare del sole e eccentricità terra
		*/
		OrbitXYZ orbit_sun_xyz = sun.orbit ( MainBody::earth_data, JD ) ;

		
	Radiant eps = epsilon;
/*
	double x_sun_eq = (-orbit_sun_xyz.x);
	double y_sun_eq = (-orbit_sun_xyz.y) * cos( eps ) - (-orbit_sun_xyz.z) * sin( eps );
	double z_sun_eq = (-orbit_sun_xyz.y) * sin( eps ) + (-orbit_sun_xyz.z) * cos( eps );
*/	
		
		
		Mercury & mercury  = data->system->mercury  ;
		OrbitXYZ orbit_mercury_xyz = mercury.orbit_eq ( MainBody::mercury_data, orbit_sun_xyz, epsilon, JD ) ;
/*		
		double x_mercury = orbit_mercury_xyz.x ;
		double y_mercury = orbit_mercury_xyz.y ;
		double z_mercury = orbit_mercury_xyz.z ;
*/		
		mercury.position = orbit_mercury_xyz ;
		
		Venus & venus  = data->system->venus  ;
		OrbitXYZ orbit_venus_xyz = venus.orbit_eq ( MainBody::venus_data, orbit_sun_xyz, epsilon, JD ) ;
		
/*		
		double x_venus = orbit_venus_xyz.x ;
		double y_venus = orbit_venus_xyz.y ;
		double z_venus = orbit_venus_xyz.z ;
*/		

		venus.position = orbit_venus_xyz ;
		

		Mars & mars  = data->system->mars  ;
//		double r_mars = mars.radius / CelestialBody::AU ;
		OrbitXYZ orbit_mars_xyz = mars.orbit_eq ( MainBody::mars_data, orbit_sun_xyz, epsilon, JD ) ;

		double x_mars = orbit_mars_xyz.x;
		double y_mars = orbit_mars_xyz.y;
		double z_mars = orbit_mars_xyz.z;
		mars.position = orbit_mars_xyz ;

		Jupiter & jupiter  = data->system->jupiter  ;
		OrbitXYZ orbit_jupiter_xyz = jupiter.orbit_eq ( MainBody::jupiter_data, orbit_sun_xyz, epsilon, JD ) ;

		double x_jupiter = orbit_jupiter_xyz.x ;
		double y_jupiter = orbit_jupiter_xyz.y ;
		double z_jupiter = orbit_jupiter_xyz.z ;
		jupiter.position = orbit_jupiter_xyz ;
		orbit_jupiter_xyz.rot_x += jupiter.obliquity ( JD ) ;
		
		Io & io = data->system->io ;
		OrbitXYZ orbit_io_xyz = io.orbit ( JD ) ;
//		double d_io = sqrt (pow(orbit_io_xyz.x,2)+pow(orbit_io_xyz.y,2)+pow(orbit_io_xyz.z,2) ) * CelestialBody::AU;
		orbit_io_xyz.x += x_jupiter ;
		orbit_io_xyz.y += y_jupiter ;
		orbit_io_xyz.z += z_jupiter ;

		// orientamento di io rispetto jupiter
		// devo ruotare di atan(y/x)
		
		io.position = orbit_io_xyz ;

		Europa & europa = data->system->europa ;
		OrbitXYZ orbit_europa_xyz = europa.orbit ( JD ) ;
//		double d_europa = sqrt( pow(orbit_europa_xyz.x,2)+pow(orbit_europa_xyz.y,2)+pow(orbit_europa_xyz.z,2) ) * CelestialBody::AU;
		orbit_europa_xyz.x += x_jupiter ;
		orbit_europa_xyz.y += y_jupiter ;
		orbit_europa_xyz.z += z_jupiter ;
		europa.position = orbit_europa_xyz ;
		
		Ganimede & ganimede = data->system->ganimede ;
		OrbitXYZ orbit_ganimede_xyz = ganimede.orbit ( JD ) ;
		orbit_ganimede_xyz.x += x_jupiter ;
		orbit_ganimede_xyz.y += y_jupiter ;
		orbit_ganimede_xyz.z += z_jupiter ;
		ganimede.position = orbit_ganimede_xyz ;
		
		Callisto & callisto = data->system->callisto ;
		OrbitXYZ orbit_callisto_xyz = callisto.orbit ( JD ) ;
		orbit_callisto_xyz.x += x_jupiter ;
		orbit_callisto_xyz.y += y_jupiter ;
		orbit_callisto_xyz.z += z_jupiter ;
		callisto.position = orbit_callisto_xyz ;



		Saturn & saturn = data->system->saturn ;
		OrbitXYZ orbit_saturn_xyz = saturn.orbit_eq ( MainBody::saturn_data, orbit_sun_xyz, epsilon, JD ) ;
/*
		double x_saturn = orbit_saturn_xyz.x ;
		double y_saturn = orbit_saturn_xyz.y ;
		double z_saturn = orbit_saturn_xyz.z ;
*/
		saturn.position = orbit_saturn_xyz ;

		Uranus & uranus = data->system->uranus ;
		OrbitXYZ orbit_uranus_xyz = uranus.orbit_eq ( MainBody::uranus_data, orbit_sun_xyz, epsilon, JD ) ;
/*

		double x_uranus = orbit_uranus_xyz.x ;
		double y_uranus = orbit_uranus_xyz.y ;
		double z_uranus = orbit_uranus_xyz.z ;
*/
		uranus.position = orbit_uranus_xyz ;

		Neptune & neptune = data->system->neptune ;
		OrbitXYZ orbit_neptune_xyz = neptune.orbit_eq ( MainBody::neptune_data, orbit_sun_xyz, epsilon, JD ) ;
/*

		double x_neptune = orbit_neptune_xyz.x ;
		double y_neptune = orbit_neptune_xyz.y ;
		double z_neptune = orbit_neptune_xyz.z ;
*/

		neptune.position = orbit_neptune_xyz ;

		
		
		// il punto di osservazione
		clientData::Camera &camera = data->camera;
		double lookat_x = camera.lookat_x;
		double lookat_y = camera.lookat_y;
		double lookat_z = camera.lookat_z;
		
		double pos_x = camera.pos_x;
		double pos_y = camera.pos_y;
		double pos_z = camera.pos_z;
		
		double up_x = camera.up_x;
		double up_y = camera.up_y;
		double up_z = camera.up_z;
		
		if ( data->lookfrom.isFromPlace ) {
			
			std::stringstream xml;
		

			double latitude = data->lookfrom.latitude;
			double longitude = data->lookfrom.longitude;
			
			if ( data->lookfrom.where == "earth" ) {
			
				Point coord = earth.bodyCoord ( latitude, longitude + rot );

				pos_x = coord.x / CelestialBody::AU;
				pos_y = coord.y / CelestialBody::AU;
				pos_z = coord.z / CelestialBody::AU;
				
				camera.up_x = pos_x;
				camera.up_y = pos_y;
				camera.up_z = pos_z;
				
				
			}
			else if ( data->lookfrom.where == "moon" ) {
			
				Point coord = moon.bodyCoord ( latitude, longitude + rot_luna );
			
				pos_x = x_moon + coord.x / CelestialBody::AU;
				pos_y = y_moon + coord.y / CelestialBody::AU;
				pos_z = z_moon + coord.z / CelestialBody::AU;
		
			}
			else if ( data->lookfrom.where == "mars" ) {
			
				Point coord = mars.bodyCoord ( latitude, longitude );
			
				pos_x = x_mars + coord.x / CelestialBody::AU;
				pos_y = y_mars + coord.y / CelestialBody::AU;
				pos_z = z_mars + coord.z / CelestialBody::AU;
		
			}
			else if ( data->lookfrom.where == "sun" ) {
			
				Point coord = sun.bodyCoord ( latitude, longitude );
			
				pos_x = x_sun + coord.x / CelestialBody::AU;
				pos_y = y_sun + coord.y / CelestialBody::AU;
				pos_z = z_sun + coord.z / CelestialBody::AU;
		
			}
			else {
			
				auto it = data->system->bodies.find( data->lookfrom.where );
				if ( it != data->system->bodies.end() ) {
				
					CelestialBody & body = it->second;
					Point coord = body.bodyCoord ( latitude, longitude );
			
					pos_x = body.position.x + coord.x / CelestialBody::AU;
					pos_y = body.position.y + coord.y / CelestialBody::AU;
					pos_z = body.position.z + coord.z / CelestialBody::AU;
				
				}
	

			}
			
		
		}

//		double scale = data->scale;
		
		if ( data->lookat.where == "earth" ) {
		
			if ( !data->lookfrom.isFromPlace ) {
			
				// se non osservi da un punto particolare allora gurda dal sole
				Point p1 (x_sun, y_sun, z_sun);
				Point p2 ( 0, 0, 0 );
			
				double r = p1.distance( p2 );
			
				StraightLine line ( p1, p2);

				Point p = line.distanceToPoint( r * 0.995 );

				pos_x = p.x ;
				pos_y = p.y ;
				pos_z = p.z ;
			
			}

			lookat_x = 0;
			lookat_y = 0;
			lookat_z = 0;
			
		}
		else if ( data->lookat.where == "earth moon" ) {
		
		
				Point p1 ( x_moon, y_moon, z_moon );
				Point p2 ( 0, 0, 0 );
			
				double r = p2.distance( p1 );
			
				StraightLine line ( p2, p1);

				Point p = line.distanceToPoint( r - r_moon  );

			pos_x = p.x  ;
			pos_y = p.y ;
			pos_z = p.z ;

			lookat_x = 0;
			lookat_y = 0;
			lookat_z = 0;
		
		}
		else if ( data->lookat.where == "sky" ) {
			
			if ( !data->lookfrom.isFromPlace ) {
			
				// se non osservi da un punto particolare allora guarda dalla terra
				/*
mi posizione sulla crosta terrestre nella linea che
unisce il centro della terra con il corpo celesto
moltiplico per 1000 in quanto i valori sono troppo piccoli
e potrebbe convergere a 0
				 */
				Point p1 ( 0, 0, 0 );
				Point p2 ( 1000 * x_moon, 1000 * y_moon, 1000 * z_moon );
			
				double r = p2.distance( p1 );
			
				StraightLine line ( p2, p1);

				Point p = line.distanceToPoint( r - 1000 * r_earth  );

				pos_x = p.x / 1000 ;
				pos_y = p.y / 1000 ;
				pos_z = p.z / 1000 ;
			
			}

		}
		else if ( data->lookat.where == "south" || data->lookat.where == "north" ) {

			double latitude = data->lookfrom.latitude;
			double longitude = data->lookfrom.longitude;
			

// vista sull'orizzonte
			double lat ;
			double lng = longitude + rot;
			if ( data->lookat.where == "south" ) {
				lat = latitude - 2 * data->camera.fov / 3 ;
			}
			else {
				lat = latitude + 2 * data->camera.fov / 3 ;
			}
			
			lat += data->lookfrom.move_lat;
			lng += data->lookfrom.move_lng;
			
			if ( lat > 90 ) lat = 90;
			if ( lat < -90 ) lat = -90;
			
					
			Sky sky;
				
			double R = sky.radius ;
					
			Radiant d = Grade( lat  );
			Radiant a = Grade( lng );
			
			double z = R * sin( d );
			double r = R * cos( d );

			double x = r * cos( a );
			double y = r * sin( a );

		
			lookat_x = x ;
			lookat_y = y ;
			lookat_z = z ;
				
				
		}
		else if ( data->lookat.where == "east" || data->lookat.where == "west" ) {

			double latitude = data->lookfrom.latitude;
			double longitude = data->lookfrom.longitude;
			

// vista sull'orizzonte
			double lat = latitude;
			double lng = longitude + rot;
			if ( data->lookat.where == "west" ) {
				lng = lng - 2 * data->camera.fov / 3 ;
			}
			else {
				lng = lng + 2 * data->camera.fov / 3 ;
			}
			
			lat += data->lookfrom.move_lat;
			lng += data->lookfrom.move_lng;
			
			Sky sky;
				
			double R = sky.radius ;
					
			Radiant d = Grade( lat  );
			Radiant a = Grade( lng );
			
			double z = R * sin( d );
			double r = R * cos( d );

			double x = r * cos( a );
			double y = r * sin( a );

		
			lookat_x = x ;
			lookat_y = y ;
			lookat_z = z ;
				
				
		}
		else if ( data->lookat.where == "zenit" ) {

			double latitude = data->lookfrom.latitude;
			double longitude = data->lookfrom.longitude;
			

// vista sull'orizzonte
			double lat = latitude;
			double lng = longitude + rot;
			
			
			lat += data->lookfrom.move_lat;
			lng += data->lookfrom.move_lng;
			
					
			Sky sky;
				
			double R = sky.radius ;
					
			Radiant d = Grade( lat  );
			Radiant a = Grade( lng );
			
			double z = R * sin( d );
			double r = R * cos( d );

			double x = r * cos( a );
			double y = r * sin( a );

		
			lookat_x = x ;
			lookat_y = y ;
			lookat_z = z ;
				
				
		}
		else if ( data->lookat.where == "sun" ) {

			lookat_x = x_sun ;
			lookat_y = y_sun ;
			lookat_z = z_sun ;
			
		}
		else {
			
			auto it = data->system->bodies.find( data->lookat.where );
			if ( it != data->system->bodies.end() ) {
				
				CelestialBody & body = it->second;
			

				if ( !data->lookfrom.isFromPlace ) {
			
// se non osservi da un punto particolare allora guarda dalla terra
				/*
mi posizione sulla crosta terrestre nella linea che
unisce il centro della terra con il corpo celesto
moltiplico per 1000 in quanto i valori sono troppo piccoli
e potrebbe convergere a 0
				 */
					double fact = 100 ;
					double r_body = body.radius / CelestialBody::AU;;
					Point p1 ( 0, 0, 0 );
					Point p2 ( fact * body.position.x, fact * body.position.y, fact * body.position.z );
			
					double r = p1.distance( p2 );
			
					StraightLine line ( p1, p2);

					Point p = line.distanceToPoint( r - fact * 100 * r_body  );

					pos_x = p.x / fact ;
					pos_y = p.y / fact ;
					pos_z = p.z / fact ;
		
			
				}

				lookat_x = body.position.x ;
				lookat_y = body.position.y ;
				lookat_z = body.position.z ;
		
			}
		
	}
		

	
		std::string hidden_sky;
		if ( data->camera.zoom < 0.1 || data->camera.zoom > 2 )
			hidden_sky = "<hidden />";	// inibisci il firmamento
		else
			hidden_sky = "";
			
		std::stringstream the_data;
		the_data.precision(12);
		the_data << "<div value=\"UT " << d.toDate( JD ) << "\" id=\"the_date\" />" <<
			 "<div value=\"JD " << JD << "\" id=\"the_jd\" />" 
			 "<input type=\"checkbox\" id=\"a_light\" value=\"" << data->ambient_light << "\" />" <<
			 "<select id=\"sel_simulazione\" value=\"" << data->simulation << "\" />" 
			 ;

		// luce ambiente
		std::string ambient_light = "";
		if ( data->ambient_light ) {
			ambient_light = "	<light name=\"ambient\" type=\"ambient\" color=\"0x323232\" />";
		}
		else {
			ambient_light = "	<light name=\"ambient\" type=\"ambient\" color=\"0x323232\"><hidden /></light>";
		}
		
		std::string terminal;
		// scrivi i dati a terminale ogni 30 sec
		if ( !timer_result.isRunning() || msecs_res > milliseconds( 30000 ) ) {
		
			std::stringstream result;
			result.precision(12);
		
			timer_result.start();
/*			

			UT ut = d.now( simulation );
			Date LT ( d.now( simulation ) + data->lookfrom.dstOffset / 24 );
			DT dt = ut ;
			
			result << 
				"UT " << ut.toDate() <<  std::endl <<
				"LT " << LT.toDate() << " (add DST if needed) " <<
				"DT " << dt.toDate()  << " " <<
				"JD " << dt.julianDay() << std::endl <<
				"from x=" << pos_x << " y=" << pos_y << " z=" << pos_z << std::endl <<
				"look x=" << lookat_x << " y=" << lookat_y << " z=" << lookat_z << std::endl ;
				
			
			for ( auto it = data->system->bodies.begin(); it != data->system->bodies.end(); ++it ) {
			
				CelestialBody & body = it->second;
				OrbitEqu o = body.position;
				double ar = o.alfa  ;
				double dec = o.delta;
				
 				if ( ar < 0 )
					ar = 360 + ar  ;

				SkyGrid & skyGrid = data->skyGrid;
				std::string constellation = skyGrid.whichConstellation ( ar, dec ) ;
//				std::string constellation  ;
				result << it->first << " in " << constellation << std::endl <<
					"Dec=" << o.delta.toGradeGPS() << " AR=" << o.alfa.toGradeHMS() << " R=" << o.R << std::endl <<
					"Dec=" << dec << " AR=" << ar << " R=" << o.R << std::endl ;
			
			}
			
*/			
			if ( result.str() != "" )
				terminal = "<terminal>" + result.str() + "</terminal>" ;
			else
				terminal = "";
		}
		
		
		


		std::stringstream e_fov, e_zoom, e_near, e_far ;
		if ( data->camera.fov != data->camera.old_fov )
			e_fov << "		<fov value=\"" << data->camera.fov << "\" />" ;
		if ( data->camera.zoom != data->camera.old_zoom )
			e_zoom << "		<zoom value=\"" << data->camera.zoom << "\" />" ;
		if ( data->camera.near != data->camera.old_near )
			e_near << "		<near value=\"" << data->camera.near << "\" />" ;
		if ( data->camera.far != data->camera.old_far )
			e_far << "		<far value=\"" << data->camera.far << "\" />" ;

		data->camera.old_fov = data->camera.fov;
		data->camera.old_zoom = data->camera.zoom;
		data->camera.old_far = data->camera.far;
		data->camera.old_near = data->camera.near;
		

		camera.lookat_x = lookat_x;
		camera.lookat_y = lookat_y;
		camera.lookat_z = lookat_z;
		
		camera.pos_x = pos_x;
		camera.pos_y = pos_y;
		camera.pos_z = pos_z;

		double FACT = data->FACT;

		std::string orbitControls = data->orbitControls ? "<orbitControls />" : "";
		
/*
		std::stringstream log;
		log << FACT * R_sky ;
		print_log(sky.primitive( o_sun_xyz ));
*/
//		OrbitXYZ sky_pos ( pos_x, pos_y, pos_z );
		OrbitXYZ sky_pos ( 0, 0, 0 );
		std::stringstream xml;
		
		xml << "<?xml version=\"1.0\" encoding=\"UTF-8\" ?>" <<  
			"<results>" << 
				terminal <<
				"<data>" << the_data.str() << "</data>" <<


"<renderer name=\"solar\" color=\"0x000000\" shadow=\"shadow\"" << " height=\"700\">" <<

orbitControls <<

"	<camera type=\"perspective\">" <<
	"	<position x=\"" << FACT * pos_x << "\" y=\"" << FACT * pos_y << "\" z=\"" << FACT * pos_z << "\" />"	<<
"		<lookat x=\"" << FACT * lookat_x << "\" y=\"" << FACT * lookat_y << "\" z=\"" << FACT * lookat_z << "\" />" <<
"		<up x=\"" << up_x << "\" y=\"" << up_y << "\" z=\"" << up_z << "\" />" <<

e_zoom.str() <<
e_fov.str() <<
e_near.str() <<
e_far.str() <<

"		<shadow />" <<
"	</camera>" <<

	"<light  type=\"point\" color=\"0xffffff\" intensity=\"1.0\" name=\"sunlight\">" <<
	"	<position x=\"" << FACT * x_sun << "\" y=\"" << FACT * y_sun << "\" z=\"" << FACT * z_sun << "\" />"	<<
	"</light>" <<
	
ambient_light <<
/*
"	<primitive type=\"sphere\" name=\"sky\" radius=\"" << FACT * R_sky << "\" widthSegments=\"30\" heightSegments=\"30\">"	<<
	"	<clickable />"	<< 
"		<rotate x=\"90\" y=\"0\" z=\"0\" />"	<<
"		<position x=\"" << 0 << "\" y=\"" << 0 << "\" z=\"" << 0 << "\" />"	<<
"		<material type=\"basic\" shininess=\"12.8\" side=\"back\" map=\"/my_image/Tycho_8162x4081_rif.jpg\">"	<<
	"		<ambient r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"		<diffuse r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"		<specular r=\"1\" g=\"1\" b=\"1\" a=\"1.0\" />"	<<
	"	</material>"	<<
	hidden_sky	<<
//	"<scale x=\"" << zoom << "\" y=\"" << zoom << "\" z=\"" << zoom << "\" />" <<
	"</primitive>"	<<
//shape.str() <<
*/

data->system->sky.primitive( sky_pos ) <<
sun.primitive( o_sun_xyz ) <<
//earth.primitive( orbit_earth_xyz ) <<
data->system->moon.primitive( o_moon_xyz ) <<

mercury.primitive( orbit_mercury_xyz ) <<
venus.primitive( orbit_venus_xyz ) <<
mars.primitive( orbit_mars_xyz ) <<
jupiter.primitive( orbit_jupiter_xyz ) <<
io.primitive( orbit_io_xyz ) <<
europa.primitive( orbit_europa_xyz ) <<
ganimede.primitive( orbit_ganimede_xyz ) <<
callisto.primitive( orbit_callisto_xyz ) <<

saturn.primitive( orbit_saturn_xyz ) <<
uranus.primitive( orbit_uranus_xyz ) <<
neptune.primitive( orbit_neptune_xyz ) <<

data->tracciato <<
data->asterismi <<
data->ecliptic <<


"</renderer>" <<


			"</results>" ;

		// manda i dati al client
		std::string type = "text";
		sendRFC6455 ( sockfd, type, xml.str(), false );
				

		
	}

// non ritrasmetto più l'informazione
data->tracciato = "" ;
data->asterismi = "" ;
	
	
}

void WSsrv::do_disconect ( Client * client ) 
{
	
	clientData * data = reinterpret_cast < clientData * > ( client->extra ); 
	delete data ;
	
	client->extra = nullptr;


#ifdef DEBUG
{

		std::stringstream ss ;
		ss << "file " << __FILE__ << " funzione " << __func__ << std::endl;
		print_log( ss.str()) ;

}
#endif

}

bool WSsrv::do_request ( Client * client ) 
{

	bool ret = true ;

	std::stringstream ss ;
	
	int sockfd = client->socket;
	std::string user = client->getQueryString("user") ;
	std::string file = client->getQueryString("file") ;
	std::string document_root = client->getQueryString("document_root") ;



	SolarSystem * system  ;
	try {
		system = new SolarSystem ( "/home/sergio/solar/solar/my_data/system.xml" ) ;
	} catch ( const std::string & str ) {
		print_log( str );
	}
	
	clientData * data = new clientData ( this, system, sockfd ); 
		
	// registra i dati extra per il client
	client->extra = data ;
	
	std::string result = "ws server is now connected";
	ss << result << std::endl;
	
	// disegna le costellazioni

/*
	Sky sky;
				
	double R = sky.radius * 0.8 ;

	data->tracciato = "";
	for ( auto it = starGrid.costellations.begin(); it != starGrid.costellations.end(); it++ ) {

		SkyMap::Constellation * c = & it->second;
		data->tracciato += c->drawBounds( R  );

	}
*/
	data->asterismi = data->skyGrid.trackAsterism ();

	sendRFC6455 ( sockfd, "text", ss.str(), false );

	return ret ;

}
