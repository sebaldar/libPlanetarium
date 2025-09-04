
#ifndef __CONFIG__
#define __CONFIG__
#include <json.hpp>
#include <sstream>

	

namespace config {

	void config_data () ;

	struct {
		
		std::string DATADIR = std::string(getenv("HOME")) + "/PLANETARIUM/webroot/data/";
		
		struct {
			std::string host = "127.0.0.1";
			std::string user = "planetarium";
			std::string psw = "eralca889190";
			std::string db = "planetarium";
		} database;
	
	} DATA;
	
}	// namespace config 


#endif
