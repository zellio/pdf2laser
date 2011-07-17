/** @file cups-epilog.c - Epilog cups driver */

/* @file cups-epilog.c @verbatim
 *========================================================================
 * E&OE Copyright © 2002-2008 Andrews & Arnold Ltd <info@aaisp.net.uk>
 * Copyright 2008 AS220 Labs <brandon@as220.org>
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *========================================================================
 *
 *
 * Author: Andrew & Arnold Ltd and Brandon Edens
 *
 *
 * Description:
 * Epilog laser engraver
 *
 * The Epilog laser engraver comes with a windows printer driver. This works
 * well with Corel Draw, and that is about it. There are other windows
 * applications, like inkscape, but these rasterise the image before sending to
 * the windows printer driver, so there is no way to use them to vector cut!
 *
 * The cups-epilog app is a cups backend, so build and link/copy to
 * /usr/lib/cups/backend/epilog. It allows you to print postscript to the laser
 * and both raster and cut. It works well with inkscape.
 *
 * With this linux driver, vector cutting is recognised by any line or curve in
 * 100% red (1.0 0.0 0.0 setrgbcolor).
 *
 * Create printers using epilog://host/Legend/options where host is the
 * hostname or IP of the epilog engraver. The options are as follows. This
 * allows you to make a printer for each different type of material.
 * af	Auto focus (0=no, 1=yes)
 * r	Resolution 75-1200
 * rs	Raster speed 1-100
 * rp	Raster power 0-100
 * vs	Vector speed 1-100
 * vp	Vector power 1-100
 * vf	Vector frequency 10-5000
 * sc	Photograph screen size in pizels, 0=threshold, +ve=line, -ve=spot, used
 *      in mono mode, default 8.
 * rm	Raster mode mono/grey/colour
 *
 * The mono raster mode uses a line or dot screen on any grey levels or
 * colours. This can be controlled with the sc parameter. The default is 8,
 * which makes a nice fine line screen on 600dpi engraving. At 600/1200 dpi,
 * the image is also lightened to allow for the size of the laser point.
 *
 * The grey raster mode maps the grey level to power level. The power level is
 * scaled to the raster power setting (unlike the windows driver which is
 * always 100% in 3D mode).
 *
 * In colour mode, the primary and secondary colours are processed as separate
 * passes, using the grey level of the colour as a power level. The power level
 * is scaled to the raster power setting. Note that red is 100% red, and non
 * 100% green and blue, etc, so 50% red, 0% green/blue is not counted as red,
 * but counts as "grey". 100% red, and 50% green/blue counts as red, half
 * power. This means you can make distinct raster areas of the page so that you
 * do not waste time moving the head over blank space between them.
 *
 * Epolog cups driver
 * Uses gs to rasterise the postscript input.
 * URI is epilog://host/Legend/options
 * E.g. epilog://host/Legend/rp=100/rs=100/vp=100/vs=10/vf=5000/rm=mono/flip/af
 * Options are as follows, use / to separate :-
 * rp   Raster power
 * rs   Raster speed
 * vp   Vector power
 * vs   Vector speed
 * vf   Vector frequency
 * w    Default image width (pt)
 * h    Default image height (pt)
 * sc   Screen (lpi = res/screen, 0=simple threshold)
 * r    Resolution (dpi)
 * af   Auto focus
 * rm   Raster mode (mono/grey/colour)
 * flip X flip (for reverse cut)
 * Raster modes:-
 * mono Screen applied to grey levels
 * grey Grey levels are power (scaled to raster power setting)
 * colour       Each colour grey/red/green/blue/cyan/magenta/yellow plotted
 * separately, lightness=power
 *
 *
 * Installation:
 * gcc -o epilog `cups-config --cflags` cups-epilog.c `cups-config --libs`
 * http://www.cups.org/documentation.php/api-overview.html
 *
 * Manual testing can be accomplished through execution akin to:
 * $ export DEVICE_URI="epilog://localhost/test"
 * # ./epilog job user title copies options
 * $ ./epilog 123 jdoe test 1 /debug < hello-world.ps
 *
 */


/*************************************************************************
 * includes
 */
#include <ctype.h>
#include <netdb.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/signal.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>


/*************************************************************************
 * local defines
 */

/** Default on whether or not auto-focus is enabled. */
#define AUTO_FOCUS (0)

/** Default bed height (y-axis) in pts. */
#define BED_HEIGHT (864)

/** Default bed width (x-axis) in pts. */
#define BED_WIDTH (1728)

/** Default for debug mode. */
#define DEBUG (1)

/** Basename for files generated by the program. */
#define FILE_BASENAME "epilog"

/** Number of characters allowable for a filename. */
#define FILENAME_NCHARS (128)

/** Default on whether or not the result is supposed to be flipped along the X
 * axis.
 */
#define FLIP (0)

/** Additional offset for the X axis. */
#define HPGLX (0)

/** Additional offset for the Y axis. */
#define HPGLY (0)

/** Accepted number of points per an inch. */
#define POINTS_PER_INCH (72)

/** Default mode for processing raster engraving (varying power depending upon
 * image characteristics).
 * Possible values are:
 * 'c' = color determines power level
 * 'g' = grey-scale levels determine power level
 * 'm' = mono mode
 * 'n' = no rasterization
 */
#define RASTER_MODE_DEFAULT 'm'

/** Default power level for raster engraving */
#define RASTER_POWER_DEFAULT (0)

/** Whether or not the raster printing is to be repeated. */
#define RASTER_REPEAT (1)

/** Default speed for raster engraving */
#define RASTER_SPEED_DEFAULT (100)

/** Default resolution is 600 DPI */
#define RESOLUTION_DEFAULT (600)

/** Pixel size of screen (0 is threshold).
 * FIXME - add more details
 */
#define SCREEN_DEFAULT (8)

/** FIXME */
#define VECTOR_FREQUENCY_DEFAULT (500)

/** Default power level for vector cutting. */
#define VECTOR_POWER_DEFAULT (50)

/** Default speed level for vector cutting. */
#define VECTOR_SPEED_DEFAULT (30)


/*************************************************************************
 * local types
 */


/*************************************************************************
 * local variables
 */

/** Determines whether or not debug is enabled. */
static char debug = DEBUG;

/** Variable to track auto-focus. */
static int focus = AUTO_FOCUS;

/** Variable to track whether or not the X axis should be flipped. */
static char flip = FLIP;

/** Height of the image (y-axis). By default this is the bed's height. */
static int height = BED_HEIGHT;

/** Job name for the print. */
static char *job_name = "";

/** User name that submitted the print job. */
static char *job_user = "";

/** Title for the job print. */
static char *job_title = "";

/** Number of copies to produce of the print. */
static char *job_copies = "";

/** Printer queue specified options. */
static char *job_options = "";

/** Variable to track the resolution of the print. */
static int resolution = RESOLUTION_DEFAULT;

/** Variable to track the mode for rasterization. One of color 'c', or
 * grey-scale 'g', mono 'm', or none 'n'
 */
static char raster_mode = RASTER_MODE_DEFAULT;

/** Variable to track the raster speed. */
static int raster_speed = RASTER_SPEED_DEFAULT;

/** Variable to track the raster power. */
static int raster_power = RASTER_POWER_DEFAULT;

/** Variable to track whether or not a rasterization should be repeated. */
static int raster_repeat = RASTER_REPEAT;

/** FIXME -- pixel size of screen, 0= threshold */
static int screen = SCREEN_DEFAULT;

/** Temporary buffer for building out strings. */
static unsigned char buf[102400];

/** Variable to track the vector speed. */
static int vector_speed = VECTOR_SPEED_DEFAULT;

/** Variable to track the vector power. */
static int vector_power = VECTOR_POWER_DEFAULT;

/** Variable to track the vector frequency. FIXME */
static int vector_freq = VECTOR_FREQUENCY_DEFAULT;

/** Width of the image (x-axis). By default this is the bed's width. */
static int width = BED_WIDTH;            // default bed


/*************************************************************************
 * local functions
 */
static bool execute_ghostscript(char *filename_bitmap, char *filename_eps,
                                char *filename_vector,
                                char *bmp_mode, int resolution,
                                int height, int width);
static bool generate_printer_job_language(FILE *bitmap_file, FILE *pjl_file,
                                          FILE *vector_file);
static bool postscript_to_encapsulated_postscript(FILE *input_file,
                                                  FILE *output_file);
static bool process_command_line_arguments(int argc, char *argv[]);
static bool process_queue_options(char *queue);
static void range_checks(void);


/*************************************************************************/

/**
 * Execute ghostscript feeding it an ecapsulated postscript file which is then
 * converted into a bitmap image. As a byproduct output of the ghostscript
 * process is redirected to a .vector file which will contain instructions on
 * how to perform a vector cut of lines within the postscript.
 *
 * @param filename_bitmap the filename to use for the resulting bitmap file.
 * @param filename_eps the filename to read in encapsulated postscript from.
 * @param filename_vector the filename that will contain the vector
 * information.
 * @param bmp_mode a string which is one of bmp16m, bmpgray, or bmpmono.
 * @param resolution the encapsulated postscript resolution.
 * @param height the postscript height in points per inch.
 * @param width the postscript width in points per inch.
 *
 * @return Return true if the execution of ghostscript succeeds, false
 * otherwise.
 */
static bool
execute_ghostscript(char *filename_bitmap,
                    char *filename_eps,
                    char *filename_vector,
                    char *bmp_mode, int resolution,
                    int height, int width)
{
    char buf[8192];
    sprintf(buf,
            "/usr/local/bin/gs -dBATCH -dNOPAUSE -r%d -g%dx%d -sDEVICE=%s \
-sOutputFile=/tmp/%s /tmp/%s > /tmp/%s",
            resolution,
            (width * resolution) / POINTS_PER_INCH,
            (height * resolution) / POINTS_PER_INCH,
            bmp_mode,
            filename_bitmap,
            filename_eps,
            filename_vector);
    if (debug) {
        fprintf(stderr, "%s\n", buf);
    }
    if (system(buf)) {
        return false;
    }
    return true;
}

/**
 *
 */
static bool
generate_printer_job_language(FILE *bitmap_file, FILE *pjl_file,
                              FILE *vector_file)
{
    int h;
    int d;
    int offx;
    int offy;
    int repx = 1;                // repeat x
    int repy = 1;                // repeat y
    int basex = 0;
    int basey = 0;

    // header
    fprintf(pjl_file, "\e%%-12345X@PJL JOB NAME=%s\r\n", job_title);
    fprintf(pjl_file, "\eE@PJL ENTER LANGUAGE=PCL\r\n");
    fprintf(pjl_file, "\e&y%dA", focus);
    fprintf(pjl_file, "\e&l0U\e&l0Z");
    //fprintf(pjl_file, "\e&y0C\e&l0U\e&l0Z");
    fprintf(pjl_file, "\e&u%dD", resolution);      // epilog, resolution
    fprintf(pjl_file, "\e*p0X\e*p0Y");      // position 0,0
    fprintf(pjl_file, "\e*t%dR", resolution);      // PCL resolution

    if (raster_power && raster_mode != 'n') {
        // raster
        int rr = raster_repeat;
        while (rr--) {
            // repeated (over printed)
            int pass;
            int passes = 1;
            long base;
            fread((char *)buf, 1, 54, bitmap_file);    // BMP header
            // Re-load width/height from bmp as possible someone used setpagedevice or some such
            width = buf[18] + (buf[19] << 8) + (buf[20] << 16) + (buf[21] << 24);
            height = buf[22] + (buf[23] << 8) + (buf[24] << 16) + (buf[25] << 24);
            base = buf[10] + (buf[11] << 8) + (buf[12] << 16) + (buf[13] << 24);
            if (raster_mode == 'c' || raster_mode == 'g') {
                h = width;       // colour/grey are byte per pixel power levels
                d = (h * 3 + 3) / 4 * 4; // BMP padded to 4 bytes per scan line
            } else {
                h = (width + 7) / 8;     // mono
                d = (h + 3) / 4 * 4;     // BMP padded to 4 bytes per scan line
            }
            if (debug) {
                fprintf(stderr, "Width %d Height %d Bytes %d Line %d\n",
                        width, height, h, d);
            }

            fprintf(pjl_file, "\e*r0F");      // raster orientation
            fprintf(pjl_file, "\e&y%dP",
                    (raster_mode == 'c' ||
                     raster_mode == 'g') ? 100 : raster_power);
            fprintf(pjl_file, "\e&z%dS", raster_speed);
            fprintf(pjl_file, "\e*r%dT", height * repy);
            fprintf(pjl_file, "\e*r%dS", width * repx);
            //fprintf(pjl_file, "\e*b%dM", (rmode == 'c' || rmode == 'g') ? 7 : 2);     // raster compression
            fprintf(pjl_file, "\e&y1O");      // raster direction, 1=up
            fprintf(pjl_file, "\e*r1A");      // start at current position
            for (offx = width * (repx - 1); offx >= 0; offx -= width) {
                for (offy = height * (repy - 1); offy >= 0; offy -= height) {
                    if (raster_mode == 'c') {
                        passes = 7;
                    }
                    for (pass = 0; pass < passes; pass++) {
                        // raster (basic)
                        int y;
                        char dir = 0;

                        fseek(bitmap_file, base, SEEK_SET);
                        for (y = height - 1; y >= 0; y--) {
                            int l;
                            switch (raster_mode) {
                            case 'c':      // colour (passes)
                            {
                                unsigned char *f = buf;
                                unsigned char *t = buf;
                                if (d > sizeof (buf)) {
                                    fprintf(stderr, "Too wide\n");
                                    return 0;
                                }
                                l = fread ((char *)buf, 1, d, bitmap_file);
                                if (l != d) {
                                    fprintf(stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
                                    return 0;
                                }
                                while (l--) {
                                    // pack and pass check RGB
                                    int n = 0;
                                    int v = 0;
                                    int p = 0;
                                    int c = 0;
                                    for (c = 0; c < 3; c++) {
                                        if (*f > 240) {
                                            p |= (1 << c);
                                        } else {
                                            n++;
                                            v += *f;
                                        }
                                        f++;
                                    }
                                    if (n) {
                                        v /= n;
                                    } else {
                                        p = 0;
                                        v = 255;
                                    }
                                    if (p != pass) {
                                        v = 255;
                                    }
                                    *t++ = 255 - v;
                                }
                            }
                            break;
                            case 'g':      // grey level
                            {
                                int d = (h + 3) / 4 * 4;  // BMP padded to 4 bytes per scan line
                                if (d > sizeof (buf)) {
                                    fprintf(stderr, "Too wide\n");
                                    return 0;
                                }
                                l = fread((char *)buf, 1, d, bitmap_file);
                                if (l != d) {
                                    fprintf (stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
                                    return 0;
                                }
                                for (l = 0; l < h; l++) {
                                    buf[l] = (255 - buf[l]);
                                }
                            }
                            break;
                            default:       // mono
                            {
                                int d = (h + 3) / 4 * 4;  // BMP padded to 4 bytes per scan line
                                if (d > sizeof (buf))
                                {
                                    fprintf(stderr, "Too wide\n");
                                    return 0;
                                }
                                l = fread((char *) buf, 1, d, bitmap_file);
                                if (l != d)
                                {
                                    fprintf(stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
                                    return 0;
                                }
                            }
                            }
                            if (raster_mode == 'c' || raster_mode == 'g') {
                                for (l = 0; l < h; l++) {
                                    /* Raster value is multiplied by the
                                     * power scale.
                                     */
                                    buf[l] = buf[l] * raster_power / 100;
                                }
                            }

                            // find left/right of data
                            for (l = 0; l < h && !buf[l]; l++) {
                                ;
                            }

                            if (l < h) {
                                // a line to print
                                int r;
                                int n;
                                unsigned char pack[sizeof (buf) * 5 / 4 + 1];
                                for (r = h - 1; r > l && !buf[r]; r--) {
                                    ;
                                }
                                r++;
                                fprintf(pjl_file, "\e*p%dY", basey + offy + y);
                                fprintf(pjl_file, "\e*p%dX", basex + offx + ((raster_mode == 'c' || raster_mode == 'g') ? l : l * 8));
                                if (dir) {
                                    fprintf(pjl_file, "\e*b%dA", -(r - l));
                                    // reverse bytes!
                                    for (n = 0; n < (r - l) / 2; n++){
                                        unsigned char t = buf[l + n];
                                        buf[l + n] = buf[r - n - 1];
                                        buf[r - n - 1] = t;
                                    }
                                } else {
                                    fprintf(pjl_file, "\e*b%dA", (r - l));
                                }
                                dir = 1 - dir;
                                // pack
                                n = 0;
                                while (l < r) {
                                    int p;
                                    for (p = l; p < r && p < l + 128 && buf[p] == buf[l]; p++);
                                    if (p - l >= 2) {
                                        // run length
                                        pack[n++] = 257 - (p - l);
                                        pack[n++] = buf[l];
                                        l = p;
                                    } else {
                                        for (p = l;
                                             p < r && p < l + 127 &&
                                                 (p + 1 == r || buf[p] !=
                                                  buf[p + 1]);
                                             p++) {
                                            ;
                                        }

                                        pack[n++] = p - l - 1;
                                        while (l < p) {
                                            pack[n++] = buf[l++];
                                        }
                                    }
                                }
                                fprintf(pjl_file, "\e*b%dW", (n + 7) / 8 * 8);
                                r = 0;
                                while (r < n)
                                    fputc(pack[r++], pjl_file);
                                while (r & 7)
                                {
                                    r++;
                                    fputc(0x80, pjl_file);
                                }
                            }
                        }
                    }
                }
            }
            fprintf(pjl_file, "\e*rC");       // end raster
            fputc(26, pjl_file);      // some end of file markers
            fputc(4, pjl_file);
        }
    } else {
        //[*r0F^[&y50P^[&z50S^[*r6600T^[*r5100S^[*r1A^[*rC^[%1B
        /* Buforary raster information. */
        fprintf(pjl_file, "\e*r0F");
        fprintf(pjl_file, "\e&y50P");
        fprintf(pjl_file, "\e&z50S");
        fprintf(pjl_file, "\e*r6600T");
        fprintf(pjl_file, "\e*r5100S");
        fprintf(pjl_file, "\e*r1A");
        fprintf(pjl_file, "\e*rC");
        fprintf(pjl_file, "\e%%1B");

    }

    if (vector_power) {
        // vector
        char up = 1;           // output status
        char newline = 1;      // input status (last was M)
        char started = 0;
        int sx = 0;
        int sy = 0;
        int lx = 0;
        int ly = 0;
        int power = 100;

        for (offy = height * (repy - 1); offy >= 0; offy -= height) {
            for (offx = width * (repx - 1); offx >= 0; offx -= width) {
                char passstart = 0;
                while (fgets((char *)buf, sizeof (buf), vector_file)) {
                    if (isalpha(*buf)) {
                        int x,
                            y;
                        if (!passstart) {
                            passstart = 1;
/*                                 fprintf(o, "IN;YP%03d;ZS%03d;XR%04d;LT", */
/*                                         vpower, vspeed, vfreq); */
/*                                 fprintf(o, "IN;YP%03d;ZS%03d;XR%04d;", */
/*                                         vpower, vspeed, vfreq); */
                            fprintf(pjl_file, "IN;");
                            fprintf(pjl_file, "XR%04d;", vector_freq);
                            fprintf(pjl_file, "YP%03d;", vector_power);
                            fprintf(pjl_file, "ZS%03d;", vector_speed);
                        }
                        switch (*buf) {
                        case 'M': // move
                            if (sscanf((char *) buf + 1, "%d,%d", &y, &x)
                                == 2) {
                                sx = x;
                                sy = y;
                                newline = 1;
                            }
                            break;
                        case 'C': // close - only makes sense after an "L"
                            if (newline == 0 && up == 0 && (lx != sx || ly
                                                            != sy)) {
                                fprintf(pjl_file, ",%d,%d;", basex + offx + sx +
                                        HPGLX, basey + offy + sy + HPGLY);
                            }
                            break;
                        case 'P': // power
                            if (sscanf((char *)buf + 1, "%d", &x) == 1 && x != power) {
                                int epower;
                                power = x;
                                if (!started) {
                                    started = 1;
                                    fprintf(pjl_file, "\e%%1B");    // start HPGL
                                }
                                if (!up) {
                                    fprintf(pjl_file, "PU");
                                }
                                up = 1;
                                epower = (power * vector_power + 50) / 100;
                                if (vector_speed && vector_speed < 100) {
                                    int espeed = vector_speed;
                                    int efreq = vector_freq;
                                    if (epower && x < 100) {
                                        int r;
                                        int q;
                                        r = 10000 / x; // power, up to set power level (i.e. x=100)
                                        q = 10000 / espeed;
                                        if (q < r)
                                            r = q;
                                        q = 500000 / efreq;
                                        if (q < r)
                                            r = q;
                                        epower = (50 + epower * r) / 100;
                                        espeed = (50 + espeed * r) / 100;
                                        efreq = (50 + espeed * r) / 100;
                                    }
                                    fprintf(pjl_file, "ZS%03d;XR%03d;", espeed, efreq);
                                }
                                fprintf(pjl_file, "YP%03d;", epower);
                            }
                            break;
                        case 'L': // line
                            if (!started) {
                                started = 1;
                                //fprintf(o, "\e%%1B;");      // start HPGL
                            }
                            if (newline) {
                                if (!up)
                                    fprintf(pjl_file, ";");
                                fprintf(pjl_file, "PU%d,%d;", basex + offx + sx + HPGLX, basey + offy + sy + HPGLY);
                                up = 1;
                                newline = 0;
                            }
/*                                 if (up) { */
/*                                     fprintf(o, "PD"); */
/*                                 } else { */
/*                                     fprintf (o, ","); */
/*                                 } */
                            up = 0;
                            if (sscanf ((char *) buf + 1, "%d,%d", &y, &x)
                                == 2) {
                                fprintf(pjl_file, "PD%d,%d;", basex + offx + x +
                                        HPGLX, basey + offy + y + HPGLY);
                            }
                            lx = x;
                            ly = y;
                            break;
                        }
                        if (*buf == 'X')
                            break;
                    }
                }
            }
        }
        if (started) {
/*                 if (up == 0) */
/*                     fprintf(o, ";"); */
            fprintf(pjl_file, "\e%%0B");      // end HLGL
        }
    }
    fprintf(pjl_file, "\e%%1BPU");  // start HLGL, and pen up, end
    // tail
    fprintf(pjl_file, "\eE");       // reset
    fprintf(pjl_file, "\e%%-12345X");       // exit language
    fprintf(pjl_file, "@PJL EOJ \r\n");     // end job
    {                         // some padding
        int i = 4096;
        while (i--) {
            fputc (0, pjl_file);
        }
    }
}

/**
 * Process the given postscript file converting it to an encapsulated
 * postscript file.
 *
 * @param input_file a file handle pointing to an opened postscript file that
 * is to be converted.
 * @param output_file a file handle pointing to the opened encapsulated
 * postscript file to store the result.
 *
 * @return Return true if the function completes its task, false otherwise.
 */
static bool
postscript_to_encapsulated_postscript(FILE *postscript_file, FILE *eps_file)
{
    int xoffset = 0;
    int yoffset = 0;

    int l;
    while (fgets((char *)buf, sizeof (buf), postscript_file)) {
        fprintf(eps_file, "%s", (char *)buf);
        if (*buf != '%') {
            break;
        }
        if (!strncasecmp((char *) buf, "%%BoundingBox:", 14)) {
            int a;
            int b;
            int c;
            int d;
            if (sscanf((char *)buf + 14, "%d %d %d %d", &a, &b, &c, &d) == 4)
            {
                xoffset = a;
                yoffset = b;
                width = (c - a);
                height = (d - b);
                fprintf(eps_file, "/setpagedevice{pop}def\n"); // use bbox
                if (xoffset || yoffset) {
                    fprintf(eps_file, "%d %d translate\n", -xoffset, -yoffset);
                }
                if (flip) {
                    fprintf(eps_file, "%d 0 translate -1 1 scale\n", width);
                }
            }
        }
        if (!strncasecmp((char *) buf, "%!", 2)) {
            fprintf
                (eps_file,
                 "/==={(        )cvs print}def/stroke{currentrgbcolor 0.0 \
eq exch 0.0 eq and exch 0.0 ne and{(P)=== currentrgbcolor pop pop 100 mul \
round  cvi = flattenpath{transform(M)=== round cvi ===(,)=== round cvi \
=}{transform(L)=== round cvi ===(,)=== round cvi =}{}{(C)=}pathforall \
newpath}{stroke}ifelse}bind def/showpage{(X)= showpage}bind def\n");
            if (raster_mode != 'c' && raster_mode != 'g') {
                if (screen == 0) {
                    fprintf(eps_file, "{0.5 ge{1}{0}ifelse}settransfer\n");
                } else {
                    int s = screen;
                    if (s < 0) {
                        s = 0 - s;
                    }
                    if (resolution >= 600) {
                        // adjust for overprint
                        fprintf(eps_file,
                                "{dup 0 ne{%d %d div add}if}settransfer\n",
                                resolution / 600, s);
                    }
                    fprintf(eps_file, "%d 30{%s}setscreen\n", resolution / s,
                            (screen > 0) ? "pop abs 1 exch sub" :
                            "180 mul cos exch 180 mul cos add 2 div");
                }
            }
        }
    }
    while ((l = fread ((char *) buf, 1, sizeof (buf), postscript_file)) > 0) {
        fwrite ((char *) buf, 1, l, eps_file);
    }
    return true;
}

/**
 * Process the command line arguments specified by the user. Proper command
 * line arguments are fed to this program from cups and will be of a form akin
 * to:
 * @verbatim
 * job_number user job_title num_copies queue_options
 * 123        jdoe myjob     1          /rp=100/rs=100/vp=100/vs=10/vf=5000
 * @endverbatim
 *
 * @param argc the number of arguments sent to the program
 * @param argv an array of strings where each element in the array represents
 * one of the options sent to the program.
 * @return Return true if the function properly completes its task, false otherwise.
 */
static bool
process_command_line_arguments(int argc, char *argv[])
{
    if (argc == 1) {
        printf("direct epilog \"Unknown\" \"Epilog laser (thin red lines vector cut)\"\n");
        return false;
    }
    if (argc > 1) {
        job_name = argv[1];
    }
    if (argc > 2) {
        job_user = argv[2];
    }
    if (argc > 3) {
        job_title = argv[3];
    }
    if (argc > 4) {
        job_copies = argv[4];
    }
    if (argc > 5) {
        job_options = argv[5];
    }
    return true;
}

/**
 * Process the queue options which take a form akin to:
 * @verbatim
 * /rp=100/rs=100/vp=100/vs=10/vf=5000/rm=mono/flip/af
 * @endverbatim
 * Each option is separated by the '/' character and can be specified as
 * /name=value or just /name (for boolean values).
 *
 * This function has the side effect of modifying the global variables
 * specifying major settings for the device.
 *
 * @param queue a string containing the options that are to be interpreted and
 * assigned to the global variables controlling printer characteristics.
 *
 * @return True if the function is able to complete its task, false otherwise.
 */
static bool
process_queue_options(char *queue_options)
{
    char *o = strchr(queue_options, '/');

    if (!queue_options) {
        fprintf(stderr, "URI syntax epilog://host/queue_optionsname\n");
        return false;
    }
    *queue_options++ = 0;

	fprintf(stderr, "options: '%s'\n", queue_options);

    if (o) {
        *o++ = 0;
        while (*o) {
            char *t = o,
                *v = "1";
            while (isalpha(*o)) {
                o++;
            }
            if (*o == '=') {
                *o++ = 0;
                v = o;
                while (*o && (isalnum(*o) || *o == '-')) {
                    o++;
                }
            }
            while (*o && !isalpha(*o)) {
                *o++ = 0;
            }
            if (!strcasecmp(t, "af")) {
                focus = atoi(v);
            }
            if (!strcasecmp(t, "r")) {
                resolution = atoi(v);
            }
            if (!strcasecmp(t, "rs")) {
                raster_speed = atoi(v);
            }
            if (!strcasecmp(t, "rp")) {
                raster_power = atoi(v);
            }
            if (!strcasecmp(t, "rm")) {
                raster_mode = tolower(*v);
            }
            if (!strcasecmp(t, "rr")) {
                raster_repeat = atoi(v);
            }
            if (!strcasecmp(t, "vs")) {
                vector_speed = atoi(v);
            }
            if (!strcasecmp(t, "vp")) {
                vector_power = atoi(v);
            }
            if (!strcasecmp(t, "vf")) {
                vector_freq = atoi(v);
            }
            if (!strcasecmp(t, "sc")) {
                screen = atoi(v);
            }
            if (!strcasecmp(t, "w")) {
                width = atoi(v);
            }
            if (!strcasecmp(t, "h")) {
                height = atoi(v);
            }
            if (!strcasecmp(t, "flip")) {
                flip = 1;
            }
            if (!strcasecmp(t, "debug")) {
                debug = 1;
            }
        }
    }
    return true;
}

/**
 * Perform range validation checks on the major global variables to ensure
 * their values are sane. If values are outside accepted tolerances then modify
 * them to be the correct value.
 *
 * @return Nothing
 */
static void
range_checks(void)
{
    if (raster_power > 100) {
        raster_power = 100;
    }
    if (raster_speed > 100) {
        raster_speed = 100;
    }
    if (vector_power > 100) {
        vector_power = 100;
    }
    if (vector_speed > 100) {
        vector_speed = 100;
    }
    if (vector_power < 0) {
        vector_power = 0;
    }
    if (vector_speed < 1) {
        vector_speed = 1;
    }
    if (raster_power < 0) {
        raster_power = 0;
    }
    if (raster_speed < 1) {
        raster_speed = 1;
    }
    if (vector_freq < 10) {
        vector_freq = 10;
    }
    if (vector_freq > 5000) {
        vector_freq = 5000;
    }
    if (resolution > 1200) {
        resolution = 1200;
    }
    if (resolution < 75) {
        resolution = 75;
    }
    if (screen < 1) {
        screen = 1;
    }
}

/**
 * Main entry point for the program.
 *
 * @param argc The number of command line options passed to the program.
 * @param argv An array of strings where each string represents a command line
 * argument.
 * @return An integer where 0 represents successful termination, any other
 * value represents an error code.
 */
int
main(int argc, char *argv[])
{
    char *site = "";
    char *queue = "";
    char host[1024] = "";

    char file_basename[FILENAME_NCHARS];
    char filename_bitmap[FILENAME_NCHARS];
    char filename_encapsulated_postscript[FILENAME_NCHARS];
    char filename_printer_job_language[FILENAME_NCHARS];
    char filename_vector[FILENAME_NCHARS];

    FILE *file_bitmap;
    FILE *file_encapsulated_postscript;
    FILE *file_postscript;
    FILE *file_printer_job_language;
    FILE *file_vector;

    int repx = 1;                // repeat x
    int repy = 1;                // repeat y
    int cenx = 0;                // x re-centre (0=not)
    int ceny = 0;                // y re-centre (0=not)
    int basex = 0;
    int basey = 0;

    gethostname(host, sizeof (host));
    {
        char *d = strchr(host, '.');
        if (d)
            *d = 0;
    }

    /* Process the command line arguments. */
    if (!process_command_line_arguments(argc, argv)) {
        return 0;
    }

    /* Gather the site information from the user's environment variable. */
    site = getenv("DEVICE_URI");
    if (!site) {
        fprintf(stderr, "No $DEVICE_URI set\n");
        return 0;
    }
    site = strstr(site, "//");
    if (!site) {
        fprintf(stderr, "URI syntax epilog://host/queuename\n");
        return 0;
    }
    site += 2;

    /* Process the queue arguments. */
    queue = strchr (site, '/');
    if (!process_queue_options(queue)) {
        fprintf(stderr, "Error processing epilog queue options.");
        return 0;
    }

    /* Perform a check over the global values to ensure that they have values
     * that are within a tolerated range.
     */
    range_checks();

    /* Determine and set the names of all files that will be manipulated by the
     * program.
     */
    sprintf(file_basename, "%s-%d", FILE_BASENAME, getpid());
    sprintf(filename_bitmap, "%s.bmp", file_basename);
    sprintf(filename_encapsulated_postscript, "%s.eps", file_basename);
    sprintf(filename_printer_job_language, "%s.pjl", file_basename);
    sprintf(filename_vector, "%s.vector", file_basename);

    /* Setup file input for postscript either given input file or standard
     * input.
     */
    if (argc > 6) {
        file_postscript = fopen(argv[6], "r");
    } else {
        file_postscript = stdin;
    }
    if (!file_postscript) {
        perror((argc > 6) ? argv[6] : "stdin");
        return 0;
    }
    /* Open the encapsulated postscript file for writing. */
    file_encapsulated_postscript = fopen(filename_encapsulated_postscript, "w");
    if (!file_encapsulated_postscript) {
        perror(filename_encapsulated_postscript);
        return 0;
    }
    /* Convert postscript to encapsulated postscript. */
    if (!postscript_to_encapsulated_postscript(file_postscript,
                                               file_encapsulated_postscript)) {
        fprintf(stderr, "Error processing postscript input to eps.");
        fclose(file_encapsulated_postscript);
        return 0;
    }
    /* Cleanup after postscript creation. */
    fclose(file_encapsulated_postscript);
    if (file_postscript != stdin) {
        fclose(file_postscript);
    }

    { // xRXxRYx for repeat, cCXcCYc is centre (mm)
        char *p = job_title;
        char *d;
        if (*p++ == 'x') {
            if (isdigit(*p)) {
                d = p;
                while (isdigit(*p)) {
                    p++;
                }
                if (*p++ == 'x') {
                    repx = atoi(d);
                    job_title = p;
                    if (isdigit(*p)) {
                        d = p;
                        while (isdigit(*p)) {
                            p++;
                        }
                        if (*p++ == 'x') {
                            repy = atoi(d);
                            job_title = p;
                        }
                    }
                }
            }
        }
        p = job_title;
        if (*p++ == 'c') {
            if (isdigit(*p)) {
                d = p;
                while (isdigit(*p)) {
                    p++;
                }
                if (*p++ == 'c') {
                    cenx = atoi(d) * 720 / 254;
                    job_title = p;
                    if (isdigit(*p)) {
                        d = p;
                        while (isdigit(*p)) {
                            p++;
                        }
                        if (*p++ == 'c') {
                            ceny = atoi(d) * 720 / 254;
                            job_title = p;
                        }
                    }
                }
            }
        }
        if (!strncmp(job_title, "nofocus", 7)) {
            job_title += 7;
            focus = 0;
        }
    }

    if (!*job_title || !strcasecmp(job_title, "_stdin_") ||
        !strcasecmp(job_title, "(stdin)") ||
        !strncasecmp (job_title, "print ", 6)) {
        static char newtitle[40];
        sprintf(newtitle, "%dx%dmm", width * 254 / 720, height * 254 / 720);
        job_title = newtitle;
    }

    if (cenx) {
        basex = cenx - width / 2;
    }
    if (ceny) {
        basey = ceny - height / 2;
    }
    if (basex < 0) {
        basex = 0;
    }
    if (basey < 0) {
        basey = 0;
    }
    // rasterises
    basex = basex * resolution / POINTS_PER_INCH;
    basey = basey * resolution / POINTS_PER_INCH;

    if(!execute_ghostscript(filename_bitmap,
                            filename_encapsulated_postscript,
                            filename_vector,
                            (raster_mode == 'c') ? "bmp16m" :
                            (raster_mode == 'g') ? "bmpgray" : "bmpmono",
                            resolution, height, width)) {
        fprintf(stderr, "Failure to execute ghostscript command.\n");
        return 1;
    }

    /* Open file handles needed by generation of the printer job language
     * file.
     */
    file_bitmap = fopen(filename_bitmap, "r");
    file_vector = fopen(filename_vector, "r");
    file_printer_job_language = fopen(filename_printer_job_language, "w");
    if (!file_printer_job_language) {
        perror(filename_printer_job_language);
        return 0;
    }
    generate_printer_job_language(file_bitmap, file_printer_job_language,
                                  file_vector);
    /* Close open file handles. */
    fclose(file_bitmap);
    fclose(file_printer_job_language);
    fclose(file_vector);


    {                            // make pjl
        FILE *i;
        FILE *o;
        int h;
        int d;
        int offx;
        int offy;
        sprintf((char *) buf, "/tmp/epilog-%d.pjl", getpid());
        o = fopen((char *) buf, "w");
        if (!o) {
            perror((char *) buf);
            return 0;
        }
        // header
        fprintf(o, "\e%%-12345X@PJL JOB NAME=%s\r\n", job_title);
        fprintf(o, "\eE@PJL ENTER LANGUAGE=PCL\r\n");
        fprintf(o, "\e&y%dA", focus);
        fprintf(o, "\e&l0U\e&l0Z");
        //fprintf(o, "\e&y0C\e&l0U\e&l0Z");
        fprintf(o, "\e&u%dD", resolution);      // epilog, resolution
        fprintf(o, "\e*p0X\e*p0Y");      // position 0,0
        fprintf(o, "\e*t%dR", resolution);      // PCL resolution


        if (raster_power && raster_mode != 'n') {
            // raster
            int rr = raster_repeat;
            while (rr--) {
                // repeated (over printed)
                int pass;
                int passes = 1;
                long base;
                sprintf((char *)buf, "/tmp/epilog-%d.bmp", getpid());
                i = fopen((char *)buf, "r");
                if (!i) {
                    perror((char *) buf);
                    return 0;        // no point retrying...
                }
                fread((char *) buf, 1, 54, i);    // BMP header
                // Re-load width/height from bmp as possible someone used setpagedevice or some such
                width = buf[18] + (buf[19] << 8) + (buf[20] << 16) + (buf[21] << 24);
                height = buf[22] + (buf[23] << 8) + (buf[24] << 16) + (buf[25] << 24);
                base = buf[10] + (buf[11] << 8) + (buf[12] << 16) + (buf[13] << 24);
                if (raster_mode == 'c' || raster_mode == 'g') {
                    h = width;       // colour/grey are byte per pixel power levels
                    d = (h * 3 + 3) / 4 * 4; // BMP padded to 4 bytes per scan line
                } else {
                    h = (width + 7) / 8;     // mono
                    d = (h + 3) / 4 * 4;     // BMP padded to 4 bytes per scan line
                }
                if (debug) {
                    fprintf(stderr, "Width %d Height %d Bytes %d Line %d\n",
                            width, height, h, d);
                }

                fprintf(o, "\e*r0F");      // raster orientation
                fprintf(o, "\e&y%dP",
                        (raster_mode == 'c' ||
                         raster_mode == 'g') ? 100 : raster_power);
                fprintf(o, "\e&z%dS", raster_speed);
                fprintf(o, "\e*r%dT", height * repy);
                fprintf(o, "\e*r%dS", width * repx);
                //fprintf(o, "\e*b%dM", (rmode == 'c' || rmode == 'g') ? 7 : 2);     // raster compression
                fprintf(o, "\e&y1O");      // raster direction, 1=up
                fprintf(o, "\e*r1A");      // start at current position
                for (offx = width * (repx - 1); offx >= 0; offx -= width) {
                    for (offy = height * (repy - 1); offy >= 0; offy -= height) {
                        if (raster_mode == 'c') {
                            passes = 7;
                        }
                        for (pass = 0; pass < passes; pass++) {
                            // raster (basic)
                            int y;
                            char dir = 0;

                            fseek(i, base, SEEK_SET);
                            for (y = height - 1; y >= 0; y--) {
                                int l;
                                switch (raster_mode) {
                                case 'c':      // colour (passes)
                                {
                                    unsigned char *f = buf;
                                    unsigned char *t = buf;
                                    if (d > sizeof (buf)) {
                                        fprintf(stderr, "Too wide\n");
                                        return 0;
                                    }
                                    l = fread ((char *) buf, 1, d, i);
                                    if (l != d) {
                                        fprintf(stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
                                        return 0;
                                    }
                                    while (l--) {
                                        // pack and pass check RGB
                                        int n = 0;
                                        int v = 0;
                                        int p = 0;
                                        int c = 0;
                                        for (c = 0; c < 3; c++) {
                                            if (*f > 240) {
                                                p |= (1 << c);
                                            } else {
                                                n++;
                                                v += *f;
                                            }
                                            f++;
                                        }
                                        if (n) {
                                            v /= n;
                                        } else {
                                            p = 0;
                                            v = 255;
                                        }
                                        if (p != pass) {
                                            v = 255;
                                        }
                                        *t++ = 255 - v;
                                    }
                                }
                                break;
                                case 'g':      // grey level
                                {
                                    int d = (h + 3) / 4 * 4;  // BMP padded to 4 bytes per scan line
                                    if (d > sizeof (buf)) {
                                        fprintf(stderr, "Too wide\n");
                                        return 0;
                                    }
                                    l = fread((char *) buf, 1, d, i);
                                    if (l != d) {
                                        fprintf (stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
                                        return 0;
                                    }
                                    for (l = 0; l < h; l++) {
                                        buf[l] = (255 - buf[l]);
                                    }
                                }
                                break;
                                default:       // mono
                                {
                                    int d = (h + 3) / 4 * 4;  // BMP padded to 4 bytes per scan line
                                    if (d > sizeof (buf))
                                    {
                                        fprintf(stderr, "Too wide\n");
                                        return 0;
                                    }
                                    l = fread((char *) buf, 1, d, i);
                                    if (l != d)
                                    {
                                        fprintf(stderr, "Bad bit data from gs %d/%d (y=%d)\n", l, d, y);
                                        return 0;
                                    }
                                }
                                }
                                if (raster_mode == 'c' || raster_mode == 'g') {
                                    for (l = 0; l < h; l++) {
                                        /* Raster value is multiplied by the
                                         * power scale.
                                         */
                                        buf[l] = buf[l] * raster_power / 100;
                                    }
                                }

                                // find left/right of data
                                for (l = 0; l < h && !buf[l]; l++) {
                                    ;
                                }

                                if (l < h) {
                                    // a line to print
                                    int r;
                                    int n;
                                    unsigned char pack[sizeof (buf) * 5 / 4 + 1];
                                    for (r = h - 1; r > l && !buf[r]; r--) {
                                        ;
                                    }
                                    r++;
                                    fprintf(o, "\e*p%dY", basey + offy + y);
                                    fprintf(o, "\e*p%dX", basex + offx + ((raster_mode == 'c' || raster_mode == 'g') ? l : l * 8));
                                    if (dir) {
                                        fprintf(o, "\e*b%dA", -(r - l));
                                        // reverse bytes!
                                        for (n = 0; n < (r - l) / 2; n++){
                                            unsigned char t = buf[l + n];
                                            buf[l + n] = buf[r - n - 1];
                                            buf[r - n - 1] = t;
                                        }
                                    } else {
                                        fprintf (o, "\e*b%dA", (r - l));
                                    }
                                    dir = 1 - dir;
                                    // pack
                                    n = 0;
                                    while (l < r) {
                                        int p;
                                        for (p = l; p < r && p < l + 128 && buf[p] == buf[l]; p++);
                                        if (p - l >= 2) {
                                            // run length
                                            pack[n++] = 257 - (p - l);
                                            pack[n++] = buf[l];
                                            l = p;
                                        } else {
                                            for (p = l;
                                                 p < r && p < l + 127 &&
                                                     (p + 1 == r || buf[p] !=
                                                      buf[p + 1]);
                                                 p++) {
                                                ;
                                            }

                                            pack[n++] = p - l - 1;
                                            while (l < p) {
                                                pack[n++] = buf[l++];
                                            }
                                        }
                                    }
                                    fprintf(o, "\e*b%dW", (n + 7) / 8 * 8);
                                    r = 0;
                                    while (r < n)
                                        fputc(pack[r++], o);
                                    while (r & 7)
                                    {
                                        r++;
                                        fputc(0x80, o);
                                    }
                                }
                            }
                        }
                    }
                }
                fclose(i);
                fprintf(o, "\e*rC");       // end raster
                fputc(26, o);      // some end of file markers
                fputc(4, o);
            }
        } else {
            //[*r0F^[&y50P^[&z50S^[*r6600T^[*r5100S^[*r1A^[*rC^[%1B
            /* Buforary raster information. */
            fprintf(o, "\e*r0F");
            fprintf(o, "\e&y50P");
            fprintf(o, "\e&z50S");
            fprintf(o, "\e*r6600T");
            fprintf(o, "\e*r5100S");
            fprintf(o, "\e*r1A");
            fprintf(o, "\e*rC");
            fprintf(o, "\e%%1B");

        }

        if (vector_power) {
            // vector
            char up = 1;           // output status
            char newline = 1;      // input status (last was M)
            char started = 0;
            int sx = 0;
            int sy = 0;
            int lx = 0;
            int ly = 0;
            int power = 100;

            sprintf((char *)buf, "/tmp/epilog-%d.vector", getpid());
            i = fopen((char *)buf, "r");

            if (!i) {
                perror((char *)buf);
                return 0;
            }

            for (offy = height * (repy - 1); offy >= 0; offy -= height) {
                for (offx = width * (repx - 1); offx >= 0; offx -= width) {
                    char passstart = 0;
                    rewind(i);
                    while (fgets((char *) buf, sizeof (buf), i)) {
                        if (isalpha(*buf)) {
                            int x,
                                y;
                            if (!passstart) {
                                passstart = 1;
/*                                 fprintf(o, "IN;YP%03d;ZS%03d;XR%04d;LT", */
/*                                         vpower, vspeed, vfreq); */
/*                                 fprintf(o, "IN;YP%03d;ZS%03d;XR%04d;", */
/*                                         vpower, vspeed, vfreq); */
                                fprintf(o, "IN;");
                                fprintf(o, "XR%04d;", vector_freq);
                                fprintf(o, "YP%03d;", vector_power);
                                fprintf(o, "ZS%03d;", vector_speed);
                            }
                            switch (*buf) {
                            case 'M': // move
                                if (sscanf((char *) buf + 1, "%d,%d", &y, &x)
                                    == 2) {
                                    sx = x;
                                    sy = y;
                                    newline = 1;
                                }
                                break;
                            case 'C': // close - only makes sense after an "L"
                                if (newline == 0 && up == 0 && (lx != sx || ly
                                                                != sy)) {
                                    fprintf (o, ",%d,%d;", basex + offx + sx +
                                             HPGLX, basey + offy + sy + HPGLY);
                                }
                                break;
                            case 'P': // power
                                if (sscanf ((char *) buf + 1, "%d", &x) == 1
                                            && x != power) {
                                    int epower;
                                    power = x;
                                    if (!started) {
                                        started = 1;
                                        fprintf (o, "\e%%1B");    // start HPGL
                                    }
                                    if (!up) {
                                        fprintf (o, "PU");
                                    }
                                    up = 1;
                                    epower = (power * vector_power + 50) / 100;
                                    if (vector_speed && vector_speed < 100) {
                                        int espeed = vector_speed;
                                        int efreq = vector_freq;
                                        if (epower && x < 100) {
                                            int r;
                                            int q;
                                            r = 10000 / x; // power, up to set power level (i.e. x=100)
                                            q = 10000 / espeed;
                                            if (q < r)
                                                r = q;
                                            q = 500000 / efreq;
                                            if (q < r)
                                                r = q;
                                            epower = (50 + epower * r) / 100;
                                            espeed = (50 + espeed * r) / 100;
                                            efreq = (50 + espeed * r) / 100;
                                        }
                                        fprintf(o, "ZS%03d;XR%03d;", espeed, efreq);
                                    }
                                    fprintf(o, "YP%03d;", epower);
                                }
                                break;
                            case 'L': // line
                                if (!started) {
                                    started = 1;
                                    //fprintf(o, "\e%%1B;");      // start HPGL
                                }
                                if (newline) {
                                    if (!up)
                                        fprintf (o, ";");
                                    fprintf(o, "PU%d,%d;", basex + offx + sx + HPGLX, basey + offy + sy + HPGLY);
                                    up = 1;
                                    newline = 0;
                                }
/*                                 if (up) { */
/*                                     fprintf(o, "PD"); */
/*                                 } else { */
/*                                     fprintf (o, ","); */
/*                                 } */
                                up = 0;
                                if (sscanf ((char *) buf + 1, "%d,%d", &y, &x)
                                    == 2) {
                                    fprintf (o, "PD%d,%d;", basex + offx + x +
                                             HPGLX, basey + offy + y + HPGLY);
                                }
                                lx = x;
                                ly = y;
                                break;
                            }
                            if (*buf == 'X')
                                break;
                        }
                    }
                }
            }
            if (started) {
/*                 if (up == 0) */
/*                     fprintf(o, ";"); */
                fprintf(o, "\e%%0B");      // end HLGL
                fclose(i);
            }
        }
        fprintf(o, "\e%%1BPU");  // start HLGL, and pen up, end
        // tail
        fprintf(o, "\eE");       // reset
        fprintf(o, "\e%%-12345X");       // exit language
        fprintf(o, "@PJL EOJ \r\n");     // end job
        {                         // some padding
            int i = 4096;
            while (i--) {
                fputc (0, o);
            }
        }
        fclose(o);
    }
    if (!debug) {
        sprintf((char *) buf, "/tmp/epilog-%d.eps", getpid ());
        if (unlink((char *) buf)) {
            perror((char *) buf);
        }
        sprintf((char *) buf, "/tmp/epilog-%d.vector", getpid ());
        if (unlink ((char *) buf)) {
            perror ((char *) buf);
        }
        sprintf((char *) buf, "/tmp/epilog-%d.bmp", getpid ());
        if (unlink ((char *) buf)) {
            perror ((char *) buf);
        }

    }
    // send by lpd
    {
        int s = -1;
        int loop = 300;
        unsigned char lpdres;
        while (loop) {
            struct addrinfo *res;
            struct addrinfo *a;
            struct addrinfo base = { 0, PF_UNSPEC, SOCK_STREAM };
            int r = getaddrinfo(site, "printer", &base, &res);
            alarm (60);
            if (!r) {
                for (a = res; a; a = a->ai_next) {
                    s = socket (a->ai_family, a->ai_socktype, a->ai_protocol);
                    if (s >= 0) {
                        if (!connect (s, a->ai_addr, a->ai_addrlen)) {
                            break;
                        } else {
                            close (s);
                            s = -1;
                        }
                    }
                }
                freeaddrinfo(res);
            }
            if (s >= 0) {
                break;
            }
            sleep(1);
            loop--;
        }
        if (!loop) {
            fprintf (stderr, "Cannot connect to %s\n", site);
            return 1;
        }
        alarm(0);
        // talk to printer
        sprintf((char *) buf, "\002%s\n", queue);
        write(s, (char *) buf, strlen ((char *) buf));
        read(s, &lpdres, 1);
        if (lpdres) {
            fprintf (stderr, "Bad response from %s, %u\n", site, lpdres);
            return 0;
        }
        sprintf((char *) buf, "H%s\nP%s\nJ%s\nldfA%s%s\nUdfA%s%s\nN%s\n", host, job_user, job_title, job_name, host, job_name, host, job_title);
        sprintf((char *) buf + strlen ((char *) buf) + 1, "\002%d cfA%s%s\n", (int) strlen ((char *) buf), job_name, host);
        write(s, buf + strlen ((char *) buf) + 1, strlen ((char *) buf + strlen ((char *) buf) + 1));
        read(s, &lpdres, 1);
        if (lpdres) {
            fprintf(stderr, "Bad response from %s, %u\n", site, lpdres);
            return 0;
        }
        write(s, (char *) buf, strlen ((char *) buf) + 1);
        read(s, &lpdres, 1);
        if (lpdres) {
            fprintf(stderr, "Bad response from %s, %u\n", site, lpdres);
            return 0;
        }
        {
            FILE *i;
            {
                struct stat s;
                sprintf((char *) buf, "/tmp/epilog-%d.pjl", getpid ());
                i = fopen((char *) buf, "r");
                //i = fopen("/tmp/brandon.pjl", "r");
                if (!i) {
                    perror((char *) buf);
                    return 0;
                }
                if (fstat(fileno (i), &s)) {
                    perror((char *) buf);
                    return 0;
                }
                sprintf((char *) buf, "\003%u dfA%s%s\n", (int) s.st_size, job_name, host);
            }
            write(s, (char *) buf, strlen((char *) buf));
            read(s, &lpdres, 1);
            if (lpdres) {
                fprintf (stderr, "Bad response from %s, %u\n", site, lpdres);
                return 0;
            }
            {
                int l;
                while ((l = fread ((char *) buf, 1, sizeof (buf), i)) > 0) {
                    write (s, (char *) buf, l);
                }
            }
            // dont wait for a response...
            fclose(i);
        }
        close(s);
    }
    if (!debug)
    {
        sprintf((char *) buf, "/tmp/epilog-%d.pjl", getpid());
        if (unlink((char *)buf))
            perror((char *)buf);
    }

    return 0;
}

