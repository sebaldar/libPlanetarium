CC      = g++
CFLAGS  = -fPIC -Wall -Wextra -std=c++17
my_home = /home/sergio
LIBNAME = libPlanetarium.so
DIR     = $(my_home)/cpp

LIBDIRS = -L$(my_home)/lib -L/usr/local/lib
LIB     = -lpthread -lcurl -lmariadbclient -lMYSQL -lXML -lJSON -lsystem

INCLUDE = -I./ \
          -I/usr/include/mysql \
          -I/usr/include/mariadb \
          -I$(DIR)/libPlanetarium/VSOP/include \
          -I$(DIR)/libSystem \
          -I$(DIR)/CMat \
          -I$(DIR)/CMat/interprete/include \
          -I$(DIR)/geometria/include \
          -I$(DIR)/libPlanetarium/timer/include \
          -I$(DIR)/libPlanetarium/date/include \
          -I$(DIR)/libPlanetarium/color \
          -I$(DIR)/libPlanetarium/bitmap \
          -I$(DIR)/libPlanetarium/webClient/include \
          -I$(DIR)/Utils \
          -I$(DIR)/libMYSQL \
          -I$(DIR)/libXML/include \
          -I$(DIR)/libJSON/include

SRCFILES = ./WSsrv.cpp \
           ./clientHttp.cpp \
           $(DIR)/Utils/Utils.cpp \
           $(DIR)/libPlanetarium/date/source/date.cpp \
           $(DIR)/libPlanetarium/webClient/source/tcp_client.cpp \
           $(DIR)/libPlanetarium/VSOP/source/vsop.cpp \
           $(DIR)/libPlanetarium/VSOP/source/elp.cpp \
           $(DIR)/libPlanetarium/VSOP/source/orbit.cpp \
           $(DIR)/libPlanetarium/VSOP/source/celestial_body.cpp \
           $(DIR)/libPlanetarium/VSOP/source/moon_jupiter.cpp \
           $(DIR)/libPlanetarium/VSOP/source/solarSystem.cpp \
           $(DIR)/libPlanetarium/VSOP/source/skymap.cpp \
           $(DIR)/libPlanetarium/VSOP/source/isInBounds.cpp \
           $(DIR)/libPlanetarium/VSOP/source/map.cpp \
           $(DIR)/libPlanetarium/VSOP/source/solarDb.cpp \
           $(DIR)/libPlanetarium/elp82b/elp82b.cpp \
           $(DIR)/libPlanetarium/elp82b/calc_interpolated_elements.cpp \
           $(DIR)/geometria/source/angolo.cpp \
           $(DIR)/geometria/source/line.cpp \
           $(DIR)/geometria/source/point.cpp \
           $(DIR)/geometria/source/plane.cpp \
           $(DIR)/geometria/source/horizont.cpp \
           $(DIR)/libPlanetarium/color/color.cpp \
           $(DIR)/libPlanetarium/timer/source/timer.cpp

OBJFILES = $(SRCFILES:.cpp=.o)

# regola principale: linka la libreria dagli .o
$(LIBNAME): $(OBJFILES)
	$(CC) $(CFLAGS) $(OBJFILES) $(LIBDIRS) $(LIB) \
	    -shared -Wl,-soname,$(LIBNAME) \
	    -o $(my_home)/lib/$(LIBNAME)

# regola generica: come ottenere un .o da un .cpp
%.o: %.cpp
	$(CC) $(CFLAGS) $(INCLUDE) -c $< -o $@

install:
	sudo ln -sf $(my_home)/lib/$(LIBNAME) /usr/local/lib/$(LIBNAME)
	sudo ldconfig

clean:
	rm -f $(OBJFILES) $(my_home)/lib/$(LIBNAME)
