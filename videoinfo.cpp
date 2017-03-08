#include "videoinfo.h"

videoInfo::videoInfo(const char* ruta, const char* nombre){
    this->ruta = ruta;
    this->nombre = nombre;
}

const char* videoInfo::getNombre(){
    return nombre;
}

const char* videoInfo::getRuta(){
    return ruta;
}

void videoInfo::setNombre(const char* nombre){
    this->nombre = nombre;
}

void videoInfo::setRuta(const char *ruta){
    this->ruta = ruta;
}

