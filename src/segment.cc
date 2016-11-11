#include <stdexcept>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <argp.h>
#include <assert.h>

#include "common.h"

using namespace std;

const char *argp_program_version = "segment 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Segment query-pv into seg-pv."; /* Program documentation. */
static char args_doc[] = "query-pv-file seg-pv-file"; /* A description of the arguments we accept. */

/* Keys for options without short-options. */
#define OPT_DEBUG       1
#define OPT_PROFILE     2
#define OPT_NETLOC_FILE 3

/* The options we understand. */
static struct argp_option options[] = {
    {"verbose",     'v',             0,             0, "produce verbose output"},
    {"quite",       'q',             0,             0, "don't produce any output"},
    {"silent",      's',             0,             OPTION_ALIAS},

    {0,0,0,0, "The following options are about output format:" },
    
    {"output",      'o',             "FILE",        0, "output information to FILE instead of standard output"},
    {"debug",       OPT_DEBUG,       0,             0, "output debug information"},
    {"profile",     OPT_PROFILE,     0,             0, "output profile information"},
    
    { 0 }
};

/* Used by main to communicate with parse_opt. */
struct arguments
{
    char *query_pv_file, *seg_pv_file, *output_file;
    bool verbose, silent, debug, profile;
};

/* Parse a single option. */
static error_t parse_opt(int key, char *arg, struct argp_state *state)
{
    struct arguments *arguments = (struct arguments *) state->input;
    switch (key)
        {
        case 'v':
            arguments->verbose = true;
            break;
        case 'q': case 's':
            arguments->silent = true;
            break;
        case 'o':
            arguments->output_file = arg;
            break;
        case OPT_DEBUG:
            arguments->debug = true;
            break;
        case OPT_PROFILE:
            arguments->profile = true;
            break;
            
        case ARGP_KEY_ARG:
            if (state->arg_num == 0) arguments->query_pv_file = arg;
            if (state->arg_num == 1) arguments->seg_pv_file = arg;
            if (state->arg_num >= 2) argp_usage(state);
            break;
            
        case ARGP_KEY_END:
            if (state->arg_num < 2) argp_usage(state);
            break;
            
        case ARGP_KEY_NO_ARGS:
            argp_usage(state);
            break;
            
        default:
            return ARGP_ERR_UNKNOWN;
        }
    return 0;
}

/* Our argp parser. */
static struct argp argp = { options, parse_opt, args_doc, prog_doc };

int main(int argc, char *argv[])
{
    struct arguments arguments;
    arguments.query_pv_file = NULL;
    arguments.seg_pv_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    regex_t record_regex = compile_regex("^(.*)\t(.*)$");
    
    qss::segmenter::Segmenter *segmenter;
    load_segmenter("./qsegconf.ini", &segmenter);

    ifstream input(arguments.query_pv_file);
    ofstream output(arguments.seg_pv_file);
    
    string line;
    while (getline(input, line)) {
        string query, pv;
        try {
            query = regex_search(&record_regex, 1, line);
            pv = regex_search(&record_regex, 2, line);
        } catch (runtime_error &e) {
            cerr << e.what() << endl;
            cerr << "line: " << line << endl;
            continue;
        }
        
        vector<string> query_seg_vec;
        segment(segmenter, query, query_seg_vec);

        if (query_seg_vec.size() == 0)
            continue;

        bool first_flag = true;
        for (vector<string>::const_iterator iter = query_seg_vec.begin(); iter != query_seg_vec.end(); ++iter) {
            if (first_flag)
                first_flag = false;
            else
                output << " ";
            output << *iter;
        }
        output << "\t" << pv << endl;
    }

    regex_free(&record_regex);
    return 0;
}
