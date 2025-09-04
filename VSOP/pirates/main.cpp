#include <string.h> // per strcmp
#include <iostream>
#include <fstream>

#include <thread>         // std::thread

#include <sys/types.h>
#include <unistd.h>

#include <solarDb.hpp>
#include "WSsrv.hpp"

// Help message for clueless users
void help() {

	std::cout << "typical: [(-d|--directory) ####] "
		<<   "[(-p|--port) ####]\n"
		<<   "[(-f|--force)]\n";
	std::cout << "  where:\n";
	std::cout << "  -d (--directory) is root directory for server to run\n";
	std::cout << "  -p (--port) is the port to connect to\n";
	std::cout << "  -f (--force) force db creation\n";

}

int main (int argc, char ** argv )
{



	std::string dir ;
	int port = 8100;
	bool force_db_creation = false;

	// A home-grown parsing algorithm starts here
	for( int i = 1; i < argc; ++i) {

		if ( strcmp(argv[i], "-d") == 0 ||

			strcmp( argv[i], "--directory") == 0) {

			if ( i + 1 == argc) {
				// error messages intermingled with parsing logic
				std::cout << "Invalid " << argv[i];
				std::cout << " parameter: no directory specified" << std::endl ;
				help();
				exit(1); // multiple exit points in parsing algorithm
			}

			dir = argv[++i];  // parsing action goes here

		}
		else if ( strcmp(argv[i], "-p") == 0 ||

			strcmp( argv[i], "--port") == 0) {

			if ( i + 1 == argc) {
				// error messages intermingled with parsing logic
				std::cout << "Invalid " << argv[i];
				std::cout << " parameter: no port specified" << std::endl ;
				help();
				exit(1); // multiple exit points in parsing algorithm
			}

			std::stringstream ss;
			ss << argv[++i];  // parsing action goes here
			ss >> port;
			
		}
		else if ( strcmp(argv[i], "-f") == 0 ||

			strcmp( argv[i], "--force") == 0) {
			force_db_creation = true;
			
		}
		else if (strcmp(argv[i], "--version") == 0) {
			std::cout << "Version 1.0" << std::endl;
			exit(0);
		}

	}

	std::string web_dir;
	if ( dir == "" ) {

		web_dir =  "/home/sergio/programmi/c++11/VSOP/pirates/webroot/";
	
	}
	else {

		web_dir = dir;

	}


    size_t last = web_dir.size() - 1 ;
    if ( web_dir.at(last) != '/' )
        web_dir += '/';

	std::cout << "try connect with dir " << web_dir << std::endl;

/*
	// crea il database, se non esiste

{
        

	try {

		TQuery q ( "localhost", "root", "eralca889190", nullptr ) ;

		q.Add("create database if not exists solar");
		q.Open();
		q.Close () ;

		q.Add("grant all privileges ON solar.* to 'sergio'@'localhost' identified by 'eralca889190'");
		q.Open();
		q.Close () ;

	}
	catch ( string e ) {
		cout << e << endl;
		exit(0);
	}

}	// fine creazione database	
*/

	if ( force_db_creation ) {
	
		try {

		/*
* attenzione occorre creare il database solar e dare i privilegi
* di accesso a sergio
create database if not exists solar
grant all privileges ON solar.* to 'sergio'@'localhost' identified by 'eralca889190'
		 */

			SolarDb db ;
			db.createStars ( );
			db.createAsterism();

		}
		catch ( string e ) {
			cout << e << endl;
			exit(0);
		}

	}

 	try {
		
		WSsrv wsServer( port, web_dir) ;
    
		std::stringstream ss;
		ss << "Server listening and accepting connections .." << std::endl;
		std::cout << ss.str() << std::endl;
        
        wsServer.print_log ( ss.str() ) ;
		
		wsServer.Accept ();

	} catch ( const std::string & e ) {

        std::cout << e << std::endl;

	} catch ( const char * e ) {
		std::cout << e << std::endl;
	}

	return ( 0 );

}
