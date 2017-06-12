# QtFfmpegProject
Aplicación de gráfica de reproducción y edición de contenido multimedia basado en las librerías FFMPEG

Desarrollada en lenguaje C++ mediante el framework Qt. 
Permite realizar diferentes acciones a los archivos multimedia como la reproducción del archivo en una interfaz gráfica preparada para tal fin, la extracción de un segmento del vídeo, la extracción por separado en diferentes archivos los canales tanto de audio como de vídeo y su posterior reproducción, escalar las dimensiones de sus frames, añadir marcas de agua, extraer información a fichero, guardado de la información en una base de datos local. 

La aplicación también cuenta con una base de datos local donde almacena la información tanto técnica como las acciones realizadas
a los archivos multimedia que se han procesado mediante la aplicación. 

Tanto las librerías creadas como la aplicación serán puestas a disposición de todo aquel que quiera adentrarse en el desarrollo 
con librerías FFmpeg y desee continuar el proyecto, ya sea siguiendo las líneas futuras del mismo o cualquier otra, 
pues en el contexto de esta línea de investigación, este desarrollo supone una base de conocimiento y de código funcional
trabajado.

#Construcción

Compilación de librerías en Linux
En el entorno Linux hay que descargar las herramienta de compilación gcc y construcción make, descargar la última versión del github oficial de Ffmpeg y extraer los ficheros. Accedemos a la carpeta y ejecutamos los mismos comandos comentados en el apartado anterior, compilamos las librerías en la carpeta del proyecto:

./configure --prefix=/home/user/Desktop/QtFFmpegProject/QtFFmpegProject/ffmpeg_build --pkg-config-flags=--static 
--extra-cflags=-I/home/user/Desktop/QtFFmpegProject/QtFFmpegProject/ffmpeg_build/include
--extra-ldflags=-L/home/user/QtFFmpegProject/QtFFmpegProject/ffmpeg_build/lib 
--bindir=/home/user/QtFFmpegProject/QtFFmpegProject/ffmpeg_build/bin 
--enable-libass --enable-avresample --enable-libfreetype 
--enable-libx264 --enable-libmp3lame --enable-libtheora --enable-libvorbis --enable-yasm --enable-gpl --enable-version3 
--enable-iconv --enable-zlib --enable-nvenc --enable-nonfree --enable-libx264
 
make -r

make install

Abrir el proyecto con Qt creator y compilar con el compilador Qt_5_8_0_GCC_64bit-Debug
