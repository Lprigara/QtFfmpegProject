#include "videodetails.h"

videoDetails::videoDetails(const char* ruta, const char* nombre){
    this->ruta = ruta;
    this->nombre = nombre;
}

const char* videoDetails::getNombre(){
    return nombre;
}

const char* videoDetails::getRuta(){
    return ruta;
}

void videoDetails::setNombre(const char* nombre){
    this->nombre = nombre;
}

void videoDetails::setRuta(const char *ruta){
    this->ruta = ruta;
}
