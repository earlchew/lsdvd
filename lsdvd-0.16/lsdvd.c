/* -*- c-basic-offset:8; indent-tabs-mode:t -*- vi: set sw=8: */
/*
 *  lsdvd.c
 *
 *  DVD info lister
 *
 *  Copyright (C) 2003  EFF
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation;
 *
 *  2003	by Chris Phillips
 *  2003-04-19  Cleanups get_title_name, added dvdtime2msec, added helper macros,
 *			  output info structures in form of a Perl module, by Henk Vergonet.
 */
#include <dvdread/ifo_read.h>
#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include "lsdvd.h"
#include "ocode.h"

static struct { char code[3];	char name[20];}
language[] = {
	{ "  ", "Not Specified" }, { "aa", "Afar" },	{ "ab", "Abkhazian" }, { "af", "Afrikaans" },	{ "am", "Amharic" },
	{ "ar", "Arabic" }, { "as", "Assamese" },	{ "ay", "Aymara" }, { "az", "Azerbaijani" }, { "ba", "Bashkir" },
	{ "be", "Byelorussian" }, { "bg", "Bulgarian" }, { "bh", "Bihari" }, { "bi", "Bislama" }, { "bn", "Bengali; Bangla" },
	{ "bo", "Tibetan" }, { "br", "Breton" }, { "ca", "Catalan" }, { "co", "Corsican" }, { "cs", "Czech" },
	{ "cy", "Welsh" }, { "da", "Dansk" }, { "de", "Deutsch" }, { "dz", "Bhutani" }, { "el", "Greek" }, { "en", "English" },
	{ "eo", "Esperanto" }, { "es", "Espanol" }, { "et", "Estonian" }, { "eu", "Basque" }, { "fa", "Persian" },
	{ "fi", "Suomi" }, { "fj", "Fiji" }, { "fo", "Faroese" }, { "fr", "Francais" }, { "fy", "Frisian" }, { "ga", "Gaelic" },
	{ "gd", "Scots Gaelic" }, { "gl", "Galician" }, { "gn", "Guarani" }, { "gu", "Gujarati" }, { "ha", "Hausa" },
	{ "he", "Hebrew" }, { "hi", "Hindi" }, { "hr", "Hrvatski" }, { "hu", "Magyar" }, { "hy", "Armenian" },
	{ "ia", "Interlingua" }, { "id", "Indonesian" }, { "ie", "Interlingue" }, { "ik", "Inupiak" }, { "in", "Indonesian" },
	{ "is", "Islenska" }, { "it", "Italiano" }, { "iu", "Inuktitut" }, { "iw", "Hebrew" }, { "ja", "Japanese" },
	{ "ji", "Yiddish" }, { "jw", "Javanese" }, { "ka", "Georgian" }, { "kk", "Kazakh" }, { "kl", "Greenlandic" },
	{ "km", "Cambodian" }, { "kn", "Kannada" }, { "ko", "Korean" }, { "ks", "Kashmiri" }, { "ku", "Kurdish" },
	{ "ky", "Kirghiz" }, { "la", "Latin" }, { "ln", "Lingala" }, { "lo", "Laothian" }, { "lt", "Lithuanian" },
	{ "lv", "Latvian, Lettish" }, { "mg", "Malagasy" }, { "mi", "Maori" }, { "mk", "Macedonian" }, { "ml", "Malayalam" },
	{ "mn", "Mongolian" }, { "mo", "Moldavian" }, { "mr", "Marathi" }, { "ms", "Malay" }, { "mt", "Maltese" },
	{ "my", "Burmese" }, { "na", "Nauru" }, { "ne", "Nepali" }, { "nl", "Nederlands" }, { "no", "Norsk" }, { "oc", "Occitan" },
	{ "om", "Oromo" }, { "or", "Oriya" }, { "pa", "Punjabi" }, { "pl", "Polish" }, { "ps", "Pashto, Pushto" },
	{ "pt", "Portugues" }, { "qu", "Quechua" }, { "rm", "Rhaeto-Romance" }, { "rn", "Kirundi" }, { "ro", "Romanian"  },
	{ "ru", "Russian" }, { "rw", "Kinyarwanda" }, { "sa", "Sanskrit" }, { "sd", "Sindhi" }, { "sg", "Sangho" },
	{ "sh", "Serbo-Croatian" }, { "si", "Sinhalese" }, { "sk", "Slovak" }, { "sl", "Slovenian" }, { "sm", "Samoan" },
	{ "sn", "Shona"  }, { "so", "Somali" }, { "sq", "Albanian" }, { "sr", "Serbian" }, { "ss", "Siswati" },
	{ "st", "Sesotho" }, { "su", "Sundanese" }, { "sv", "Svenska" }, { "sw", "Swahili" }, { "ta", "Tamil" },
	{ "te", "Telugu" }, { "tg", "Tajik" }, { "th", "Thai" }, { "ti", "Tigrinya" }, { "tk", "Turkmen" }, { "tl", "Tagalog" },
	{ "tn", "Setswana" }, { "to", "Tonga" }, { "tr", "Turkish" }, { "ts", "Tsonga" }, { "tt", "Tatar" }, { "tw", "Twi" },
	{ "ug", "Uighur" }, { "uk", "Ukrainian" }, { "ur", "Urdu" }, { "uz", "Uzbek" }, { "vi", "Vietnamese" },
	{ "vo", "Volapuk" }, { "wo", "Wolof" }, { "xh", "Xhosa" }, { "yi", "Yiddish" }, { "yo", "Yoruba" }, { "za", "Zhuang" },
	{ "zh", "Chinese" }, { "zu", "Zulu" }, { "xx", "Unknown" }, { "\0", "Unknown" } };
char *video_format[2] = {"NTSC", "PAL"};
/* 28.9.2003: Chicken run's aspect ratio is 16:9 or 1.85:1, at index
   1.  Addionaly using ' in the quoting makes the perl output not
   parse so changed to " */
char *aspect_ratio[4] = {"4/3", "16/9", "\"?:?\"", "16/9"};
char *quantization[4] = {"16bit", "20bit", "24bit", "drc"};
char *mpeg_version[2] = {"mpeg1", "mpeg2"};
/* 28.9.2003: The European chicken run title has height index 3, and
   576 lines seems right, mplayer -vop cropdetect shows from 552 to
   576 lines.  What the correct value is for index 2 is harder to say */
char *video_height[4] = {"480", "576", "???", "576"};
char *video_width[4]  = {"720", "704", "352", "352"};
char *permitted_df[4] = {"P&S + Letter", "Pan&Scan", "Letterbox", "?"};
char *audio_format[7] = {"ac3", "?", "mpeg1", "mpeg2", "lpcm ", "sdds ", "dts"};
int   audio_id[7]     = {0x80, 0, 0xC0, 0xC0, 0xA0, 0, 0x88};
/* 28.9.2003: Chicken run again, it has frequency index of 1.
   According to dvd::rip the frequency is 48000 */
char *sample_freq[2]  = {"48000", "48000"};
char *audio_type[5]   = {"Undefined", "Normal", "Impaired", "Comments1", "Comments2"};
char *subp_type[16]   = {"Undefined", "Normal", "Large", "Children", "reserved", "Normal_CC", "Large_CC", "Children_CC",
	"reserved", "Forced", "reserved", "reserved", "reserved", "Director", "Large_Director", "Children_Director"};
double frames_per_s[4] = {-1.0, 25.00, -1.0, 29.97};
int   subp_id_shift[4] = {24, 8, 8, 8};

char* program_name;

//extern void operl_print(struct dvd_info *dvd_info);
//extern void oxml_print(struct dvd_info *dvd_info);
//extern void oruby_print(struct dvd_info *dvd_info);
//extern void ohuman_print(struct dvd_info *dvd_info);

/* Ticks
 *
 * Compute durations using rational arithmetic to avoid loss of precision
 * when summing. The result of the sum will be exact prior to converting
 * the sum.
 */
#define TICK_SCALE 1001

typedef struct {
	unsigned ticks;
	unsigned scale;
} dvd_ticks_t;

dvd_ticks_t dvdtime2ticks(const dvd_time_t *dt)
  {
	unsigned scale = 0;
	unsigned secs  = 0;
	unsigned ticks = 0;

	if (dt) {
		static const unsigned tick_scale[4] = {
			0, 25 * TICK_SCALE, 0, 30 * (TICK_SCALE-1) };

		scale = tick_scale[(dt->frame_u & 0xc0) >> 6];

		secs += (((dt->hour   & 0xf0) >> 3) * 5 + (dt->hour   & 0x0f)) * 3600;
		secs += (((dt->minute & 0xf0) >> 3) * 5 + (dt->minute & 0x0f)) * 60;
		secs += (((dt->second & 0xf0) >> 3) * 5 + (dt->second & 0x0f)) * 1;

		ticks += ((dt->frame_u & 0x30)>> 3) * 5 + (dt->frame_u & 0x0f);
		ticks *= TICK_SCALE;
	}

	return (dvd_ticks_t) {
		.scale = scale,
		.ticks = secs * scale + ticks,
	};
  }

dvd_ticks_t addticks(dvd_ticks_t lhs, dvd_ticks_t rhs)
  {
	if (lhs.ticks == 0)
		return rhs;
	if (rhs.ticks == 0)
		return lhs;
	return (dvd_ticks_t) {
		.ticks = lhs.ticks + rhs.ticks,
		.scale = lhs.scale == rhs.scale ? lhs.scale : 0,
	};
}

playback_time_t ticks2playbacktime(dvd_ticks_t dt)
{
	unsigned secs = dt.ticks / dt.scale;
	unsigned frac = dt.ticks % dt.scale;

	return (playback_time_t) {
		.hour   = (secs / 3600),
		.minute = (secs % 3600) / 60,
		.second = (secs %   60),
		.usec   = (frac * (1000000 / dt.scale) +
			   frac * (1000000 % dt.scale) / dt.scale),
		.ticks  = (dt.ticks),
		.scale  = (dt.scale),
	};
}

double playbacktime(playback_time_t pt)
{
	return pt.hour   * 3600 +
	       pt.minute *   60 +
	       pt.second *    1 +
	       pt.usec   /  1e6;
}

/*
 *  The following method is based on code from vobcopy, by Robos, with thanks.
 */
int get_title_name(const char* dvd_device, char* title)
{
	FILE *filehandle = 0;
	int  i;

	if (! (filehandle = fopen(dvd_device, "r"))) {
		fprintf(stderr, "Couldn't open %s for title\n", dvd_device);
		strcpy(title, "unknown");
		return -1;
	}

	if ( fseek(filehandle, 32808, SEEK_SET )) {
		fclose(filehandle);
		fprintf(stderr, "Couldn't seek in %s for title\n", dvd_device);
		strcpy(title, "unknown");
		return -1;
	}

	if ( 32 != (i = fread(title, 1, 32, filehandle)) ) {
		fclose(filehandle);
		fprintf(stderr, "Couldn't read enough bytes for title.\n");
		strcpy(title, "unknown");
		return -1;
	}

	fclose (filehandle);

	title[32] = '\0';
	while(i-- > 2)
	if(title[i] == ' ') title[i] = '\0';
	return 0;
}

char* lang_name(char* code)
{
	int k=0;
	while (memcmp(language[k].code, code, 2) && language[k].name[0] ) { k++; }
	return language[k].name;
}

void version()
{
	fprintf(stderr, "lsdvd "VERSION" - GPL Copyright (c) 2002, 2003, 2004, 2005 \"Written\" by Chris Phillips <acid_kewpie@users.sf.net>\n");
}

void usage()
{
	version();
	fprintf(stderr, "Usage: %s [options] [-t track_number] [dvd path] \n", program_name);
	fprintf(stderr, "\n");
	fprintf(stderr, "Options:\n");
	fprintf(stderr, "\tExtra information:\n");
	fprintf(stderr, "\t  -a audio streams\n");
	fprintf(stderr, "\t  -d cells\n");
	fprintf(stderr, "\t  -n angles\n");
	fprintf(stderr, "\t  -c chapters\n");
	fprintf(stderr, "\t  -s subpictures\n");
	fprintf(stderr, "\t  -P palette\n");
	fprintf(stderr, "\t  -v video\n");
	fprintf(stderr, "\t  -x all information\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\tFormatting:\n");
	fprintf(stderr, "\t  -Oh output as human readable (default)\n");
	fprintf(stderr, "\t  -Op output as Perl\n");
	fprintf(stderr, "\t  -Oy output as Python\n");
	fprintf(stderr, "\t  -Or output as Ruby\n");
	fprintf(stderr, "\t  -Ox output as XML\n");
	//fprintf(stderr, "\t  -p output as Perl [deprecated]\n");
	fprintf(stderr, "\n");
	fprintf(stderr, "\tOther options:\n");
	fprintf(stderr, "\t  -q quiet - no summary totals\n");
	fprintf(stderr, "\t  -h this message\n");
	fprintf(stderr, "\t  -V version information\n");
	fprintf(stderr, "\n");
}

int opt_a=0, opt_c=0, opt_n=0, opt_p=0, opt_q=0, opt_s=0, opt_t=0, opt_v=0, opt_x=0, opt_d=0, opt_P=0;
char opt_O='h';

char output_option(char *arg)
{
	if (strlen(arg) == 1) {
		return arg[0];
	} else if (strcmp(arg, "perl") == 0) {
		return 'p';
	} else if (strcmp(arg, "python") == 0) {
		return 'y';
	} else if (strcmp(arg, "ruby") == 0) {
		return 'r';
	} else if (strcmp(arg, "ruby2") == 0) {
		return '2';
	} else if (strcmp(arg, "xml") == 0) {
		return 'x';
	} else if (strcmp(arg, "human") == 0) {
		return 'h';
	} else {
		return '\0';
	}
}


int main(int argc, char *argv[])
{
	char title[33];
	dvd_reader_t *dvd;
	ifo_handle_t *ifo_zero, **ifo;
	pgcit_t *vts_pgcit;
	vtsi_mat_t *vtsi_mat;
	vmgi_mat_t *vmgi_mat;
	audio_attr_t *audio_attr;
	video_attr_t *video_attr;
	subp_attr_t *subp_attr;
	pgc_t *pgc;
	int i, j, k, c, titles, vts_ttn, title_set_nr;
	char lang_code[3];
	char *dvd_device = "/dev/dvd";
	int has_title = 0, ret = 0;
	int max_length = 0, max_track = 0;
	struct stat dvd_stat;

	program_name = argv[0];

	while ((c = getopt(argc, argv, "acnpPqsdvt:O:xhV?")) != EOF) {
		switch (c) {
		case 'h':
		case '?':	usage();		return 0;
		case 'V':	version();		return 0;
		case 'a':	opt_a = 1;		break;
		case 'd':	opt_d = 1;		break;
		case 's':	opt_s = 1;		break;
		case 'q':	opt_q = 1;		break;
		case 'c':	opt_c = 1;		break;
		case 'n':	opt_n = 1;		break;
		case 'p':	opt_p = 1;		break;
		case 'P':	opt_P = 1;		break;
		case 't':	opt_t = atoi(optarg);	break;
		case 'O':	opt_O = output_option(optarg);	break;
		case 'v':	opt_v = 1;		break;
		case 'x':	opt_x = 1;
				opt_a = 1;
				opt_c = 1;
				opt_s = 1;
				opt_P = 1;
				opt_d = 1;
				opt_n = 1;
				opt_v = 1;		break;
		}
	}

	if (argv[optind]) { dvd_device = argv[optind];	}

	ret = stat(dvd_device, &dvd_stat);
	if ( ret < 0 ) {
		fprintf(stderr, "Can't find device %s\n", dvd_device);
		return 1;
	}

	dvd = DVDOpen(dvd_device);
	if( !dvd ) {
		fprintf( stderr, "Can't open disc %s!\n", dvd_device);
		return 2;
	}
	ifo_zero = ifoOpen(dvd, 0);
	if ( !ifo_zero ) {
		fprintf( stderr, "Can't open main ifo!\n");
		return 3;
	}

	ifo = (ifo_handle_t **)malloc((ifo_zero->vts_atrt->nr_of_vtss + 1) * sizeof(ifo_handle_t *));

	for (i=1; i <= ifo_zero->vts_atrt->nr_of_vtss; i++) {
		ifo[i] = ifoOpen(dvd, i);
		if ( !ifo[i] && opt_t == i ) {
			fprintf( stderr, "Can't open ifo %d!\n", i);
			return 4;
		}
	}

	titles = ifo_zero->tt_srpt->nr_of_srpts;

	if ( opt_t > titles || opt_t < 0) {
		fprintf (stderr, "Only %d titles on this disc!", titles);
		return 5;
	}

	has_title = get_title_name(dvd_device, title);

	vmgi_mat = ifo_zero->vmgi_mat;

	struct dvd_info dvd_info;

	dvd_info.discinfo.device = dvd_device;
	dvd_info.discinfo.disc_title = has_title ? "unknown" : title;
	dvd_info.discinfo.vmg_id =  vmgi_mat->vmg_identifier;
	dvd_info.discinfo.provider_id = vmgi_mat->provider_identifier;

	dvd_info.title_count = titles;
	dvd_info.titles = calloc(titles, sizeof(*dvd_info.titles));

	for (j=0; j < titles; j++)
	{

	if ( opt_t == j+1 || opt_t == 0 ) {

	// GENERAL
	if (ifo[ifo_zero->tt_srpt->title[j].title_set_nr]->vtsi_mat) {

		dvd_info.titles[j].enabled = 1;

		vtsi_mat   = ifo[ifo_zero->tt_srpt->title[j].title_set_nr]->vtsi_mat;
		vts_pgcit  = ifo[ifo_zero->tt_srpt->title[j].title_set_nr]->vts_pgcit;
		video_attr = &vtsi_mat->vts_video_attr;
		vts_ttn = ifo_zero->tt_srpt->title[j].vts_ttn;
		vmgi_mat = ifo_zero->vmgi_mat;
		title_set_nr = ifo_zero->tt_srpt->title[j].title_set_nr;
		pgc = vts_pgcit->pgci_srp[ifo[title_set_nr]->vts_ptt_srpt->title[vts_ttn - 1].ptt[0].pgcn - 1].pgc;
		dvd_info.titles[j].general.length =
			playbacktime(
				ticks2playbacktime(
					dvdtime2ticks(&pgc->playback_time)));
		dvd_info.titles[j].general.playback_time =
			ticks2playbacktime(dvdtime2ticks(&pgc->playback_time));
		dvd_info.titles[j].general.vts_id = vtsi_mat->vts_identifier;

		if (dvd_info.titles[j].general.length > max_length) {
			max_length = dvd_info.titles[j].general.length;
			max_track = j+1;
		}

		dvd_info.titles[j].chapter_count_reported = ifo_zero->tt_srpt->title[j].nr_of_ptts;
		dvd_info.titles[j].cell_count = pgc->nr_of_cells;

		dvd_info.titles[j].audiostream_count = 0;
                for (k=0; k < 8; k++)
                  if (pgc->audio_control[k] & 0x8000)
                    dvd_info.titles[j].audiostream_count++;

                dvd_info.titles[j].subtitle_count = 0;
                for (k=0; k < 32; k++)
                  if (pgc->subp_control[k] & 0x80000000)
                    dvd_info.titles[j].subtitle_count++;

		if(opt_v) {
			dvd_info.titles[j].parameter.vts = ifo_zero->tt_srpt->title[j].title_set_nr;
			dvd_info.titles[j].parameter.ttn = ifo_zero->tt_srpt->title[j].vts_ttn;
			dvd_info.titles[j].parameter.fps = frames_per_s[(pgc->playback_time.frame_u & 0xc0) >> 6];
			dvd_info.titles[j].parameter.format = video_format[video_attr->video_format];
			dvd_info.titles[j].parameter.aspect = aspect_ratio[video_attr->display_aspect_ratio];
			dvd_info.titles[j].parameter.width = video_width[video_attr->picture_size];
			dvd_info.titles[j].parameter.height = video_height[video_attr->video_format];
			dvd_info.titles[j].parameter.df_code = video_attr->permitted_df;

		}

		// PALETTE
		if (opt_P) {
			dvd_info.titles[j].palette = malloc(16 * sizeof(int));
			for (i=0; i < 16; i++) { dvd_info.titles[j].palette[i] = pgc->palette[i]; }
		} else {
			dvd_info.titles[j].palette = NULL;
		}

		// ANGLES

		if (opt_n) {
			dvd_info.titles[j].angle_count = ifo_zero->tt_srpt->title[j].nr_of_angles;
		} else {
			dvd_info.titles[j].angle_count = 0;
		}

		// AUDIO

		if (opt_a) {

			dvd_info.titles[j].audiostreams = calloc(dvd_info.titles[j].audiostream_count, sizeof(*dvd_info.titles[j].audiostreams));

			for (i=0, k=0; i<8; i++)
			{
                                if ((pgc->audio_control[i] & 0x8000) == 0) continue;

                                audio_attr = &vtsi_mat->vts_audio_attr[i];
				sprintf(lang_code, "%c%c", audio_attr->lang_code>>8, audio_attr->lang_code & 0xff);
				if (!isalpha(lang_code[0]) || !isalpha(lang_code[1])) { lang_code[0] = 'x'; lang_code[1] = 'x'; }

				dvd_info.titles[j].audiostreams[k].langcode = strdup(lang_code);
				dvd_info.titles[j].audiostreams[k].language = lang_name(lang_code);
				dvd_info.titles[j].audiostreams[k].format = audio_format[audio_attr->audio_format];
				dvd_info.titles[j].audiostreams[k].frequency = sample_freq[audio_attr->sample_frequency];
				dvd_info.titles[j].audiostreams[k].quantization = quantization[audio_attr->quantization];
				dvd_info.titles[j].audiostreams[k].channels = audio_attr->channels+1;
				dvd_info.titles[j].audiostreams[k].ap_mode = audio_attr->application_mode;
				dvd_info.titles[j].audiostreams[k].content = audio_type[audio_attr->lang_extension];
				dvd_info.titles[j].audiostreams[k].streamid = audio_id[audio_attr->audio_format] + (pgc->audio_control[i] >> 8 & 7);
                                k++;
			}
		} else {
			dvd_info.titles[j].audiostreams = NULL;
		}

		// CHAPTERS

		if (opt_c) {
			int cell = pgc->program_map[0] - 1;
			int end = cell + pgc->nr_of_cells;

			dvd_info.titles[j].chapter_count = pgc->nr_of_programs;
			dvd_info.titles[j].chapters = calloc(dvd_info.titles[j].chapter_count, sizeof(*dvd_info.titles[j].chapters));

			for (i=0; i<pgc->nr_of_programs; i++)
			{
				dvd_ticks_t ticks = dvdtime2ticks(NULL);
				int next =
					(i == pgc->nr_of_programs - 1)
					? end
					: pgc->program_map[i+1] - 1;
				while (cell < next)
				{
                                        // Only use first cell of multi-angle cells
                                        if (pgc->cell_playback[cell].block_mode <= 1)
                                        {
						dvd_ticks_t dt = dvdtime2ticks(
							&pgc->cell_playback[cell].playback_time);
						ticks = addticks(ticks, dt);
						dvd_info.titles[j].chapters[i].playback_time =
							ticks2playbacktime(dt);
                                        }
					cell++;
				}
				dvd_info.titles[j].chapters[i].startcell = pgc->program_map[i];
				dvd_info.titles[j].chapters[i].lastcell = cell;
				dvd_info.titles[j].chapters[i].length =
					playbacktime(
						ticks2playbacktime(ticks));
			}
		}

		// CELLS


		dvd_info.titles[j].cells = calloc(dvd_info.titles[j].cell_count, sizeof(*dvd_info.titles[j].cells));

		if (opt_d) {
			for (i=0; i<pgc->nr_of_cells; i++)
			{
				dvd_ticks_t dt = dvdtime2ticks(
					&pgc->cell_playback[i].playback_time);
				playback_time_t pt = ticks2playbacktime(dt);

				dvd_info.titles[j].cells[i].playback_time = pt;
				dvd_info.titles[j].cells[i].length =
					playbacktime(pt);

                                dvd_info.titles[j].cells[i].block_mode = pgc->cell_playback[i].block_mode;
                                dvd_info.titles[j].cells[i].block_type = pgc->cell_playback[i].block_type;
				dvd_info.titles[j].cells[i].first_sector = pgc->cell_playback[i].first_sector;
				dvd_info.titles[j].cells[i].last_sector = pgc->cell_playback[i].last_sector;
			}
		} else {
			dvd_info.titles[j].cells = NULL;
		}

		// SUBTITLES

		dvd_info.titles[j].subtitles = calloc(dvd_info.titles[j].subtitle_count, sizeof(*dvd_info.titles[j].subtitles));

		if (opt_s) {
			for (i=0, k=0; i<32; i++)
			{
                                if ((pgc->subp_control[i] & 0x80000000) == 0) continue;

				subp_attr = &vtsi_mat->vts_subp_attr[i];
				sprintf(lang_code, "%c%c", subp_attr->lang_code>>8, subp_attr->lang_code & 0xff);
				if (!isalpha(lang_code[0]) || !isalpha(lang_code[1])) { lang_code[0] = 'x'; lang_code[1] = 'x'; }

				dvd_info.titles[j].subtitles[k].langcode = strdup(lang_code);
				dvd_info.titles[j].subtitles[k].language = lang_name(lang_code);
				dvd_info.titles[j].subtitles[k].content = subp_type[subp_attr->lang_extension];
				dvd_info.titles[j].subtitles[k].streamid = 0x20 + ((pgc->subp_control[i] >> subp_id_shift[video_attr->display_aspect_ratio]) & 0x1f);
                                k++;
			}
		} else {
			dvd_info.titles[j].subtitles = NULL;
		}

	} // if vtsi_mat
	} // if not -t
	} // for each title

	if (! opt_t) { dvd_info.longest_track = max_track; }

	if (opt_p) {
		ocode_print(&perl_syntax, &dvd_info);
	} else {
		switch(opt_O) {
			case 'p':
				ocode_print(&perl_syntax, &dvd_info);
				break;
                        case 'y':
			       ocode_print(&python_syntax, &dvd_info);
				break;
			case 'x':
				oxml_print(&dvd_info);
				break;
			case 'r':
				ocode_print(&ruby_syntax, &dvd_info);
				break;
                        case 'd':
				ocode_print(&debug_syntax, &dvd_info);
				break;
			default :
				ohuman_print(&dvd_info);
				break;
		}
	}

	for (i=1; i <= ifo_zero->vts_atrt->nr_of_vtss; i++) { ifoClose(ifo[i]);	}

	ifoClose(ifo_zero);
	DVDClose(dvd);

	return 0;
}
