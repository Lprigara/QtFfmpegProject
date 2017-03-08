#ifndef VIDEODETAILS_H
#define VIDEODETAILS_H


class videoDetails
{
public:
    videoDetails(const char* ruta, const char* nombre);
    void setRuta(const char* ruta);
    void setNombre(const char* nombre);
    const char* getRuta();
    const char* getNombre();

private:
    const char* ruta;
    const char* nombre;
};

#endif // VIDEODETAILS_H
