CC=g++
#CFLAGS=-fPIC -Wall -Wextra -std=c++11 -DDEBUG
CFLAGS=-fPIC -Wall -Wextra -std=c++11
#CFLAGS_DEBUG=-g -DDEBUG

APPNAME=pirates
DIR11=$(HOME)/programmi/c++11
DIR=$(HOME)/programmi/c++

LIB=-lXML -lmysqlclient -lpng -lpthread

INCLUDE=	-I./		\
		-I../include		\
		-I/usr/include/mysql	\
		-I$(DIR11)/json					\
		-I$(DIR11)/CMat					\
		-I$(DIR11)/CMat/interprete/include	\
		-I$(DIR11)/geometria/include	\
		-I$(DIR11)/timer/include	\
		-I$(DIR11)/date/include	\
		-I$(DIR11)/angolo	\
		-I$(DIR11)/webClient/include	\
		-I$(DIR)/crypt/include 	\
		-I$(DIR)/Server/include		\
		-I$(DIR)/mysql				\
		-I$(DIR)/Utils				\
		-I$(DIR)/xml/include		

WSsrv=./WSsrv.cpp
server=$(DIR)/Server/source/WebServerBase.cpp
client=$(DIR)/Server/source/clientHttp.cpp
webclient=$(DIR11)/webClient/source/tcp_client.cpp
ws=$(DIR)/Server/source/WebServerWS.cpp
sha1=$(DIR)/crypt/source/sha1.cpp
base64=$(DIR)/crypt/source/base64.cpp
utils=$(DIR)/Utils/Utils.cpp
timer=$(DIR11)/timer/source/timer.cpp
vsop=../source/vsop.cpp
elp=../source/elp.cpp
orbit=../source/orbit.cpp
celestial=../source/celestial_body.cpp	
moon_jupiter=../source/moon_jupiter.cpp	
solar=../source/solarSystem.cpp	
skymap=../source/skymap.cpp	
map=../source/map.cpp	
solardb=../source/solarDb.cpp	
date=$(DIR11)/date/source/date.cpp
angolo=$(DIR11)/angolo/angolo.cpp
elp82b=$(DIR11)/elp82b/elp82b.cpp
calc_interpolated_elements=$(DIR11)/elp82b/calc_interpolated_elements.cpp

mat=$(DIR11)/CMat/cmat.cpp
variable=$(DIR11)/CMat/variabile.cpp
funzione=$(DIR11)/CMat/funzione.cpp
matrix=$(DIR11)/CMat/matrix.cpp

line=$(DIR11)/geometria/source/line.cpp
point=$(DIR11)/geometria/source/point.cpp

mysql=$(DIR)/mysql/tmysql.cpp


main=./main.cpp

SRCFILES=$(server) $(client) $(utils) $(WSsrv) $(ws) 	\
	$(sha1) $(base64) $(timer) 	$(angolo)	\
	$(elp82b) $(calc_interpolated_elements) 	\
	$(vsop) $(celestial) $(moon_jupiter) $(solar) $(date) \
	$(elp)	$(orbit)	$(skymap)	$(map)			\
	$(mat) $(solardb) $(variable) $(funzione) $(matrix)	\
	$(line) $(point)	$(webclient) $(mysql) \
	$(main)

$(APPNAME) : $(SRCFILES)
	   $(CC) $(CFLAGS) $(CFLAGS_DEBUG)  $(INCLUDE) \
	   $(SRCFILES) \
	   -o $(HOME)/bin/$(APPNAME) $(LIB)




