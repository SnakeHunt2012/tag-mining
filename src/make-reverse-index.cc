#include <stdexcept>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <argp.h>
#include <assert.h>

#include "FastTrie.h"
#include "common.h"

#define MAX_CMD_WORD_NUM 1024

using namespace std;
using namespace ft2;

const char *argp_program_version = "make-reverse-index 0.1";
const char *argp_program_bug_address = "<SnakeHunt2012@gmail.com>";

static char prog_doc[] = "Make reverse index (url->[tag ...]) using fasttrie."; /* Program documentation. */
static char args_doc[] = "TREE_FILE URL_TITLE_FILE INDEX_FILE"; /* A description of the arguments we accept. */

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
    char *tree_file, *url_title_file, *index_file, *output_file;
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
            if (state->arg_num == 0) arguments->tree_file = arg;
            if (state->arg_num == 1) arguments->url_title_file = arg;
            if (state->arg_num == 2) arguments->index_file = arg;
            if (state->arg_num >= 3) argp_usage(state);
            break;
            
        case ARGP_KEY_END:
            if (state->arg_num < 3) argp_usage(state);
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

int retrieve_word(Container<Trie<String, int> > &trie_tree, const string &cand_str, vector<string> &retrieveWords);


int main(int argc, char *argv[])
{
    struct arguments arguments;
    arguments.tree_file = NULL;
    arguments.url_title_file = NULL;
    arguments.index_file = NULL;
    arguments.output_file = NULL;
    arguments.verbose = true;
    arguments.silent = false;
    arguments.debug = false;
    arguments.profile = false;
    argp_parse(&argp, argc, argv, 0, 0, &arguments);

    
    ifstream input(arguments.url_title_file);

    Container<Trie<String, int> > trie_tree = Container<Trie<String, int> > (arguments.tree_file);
    
    regex_t record_regex = compile_regex("^(.*)\t(.*)$");
    regex_t category_regex = compile_regex("^([^|]*)");
    
    qss::segmenter::Segmenter *segmenter;
    load_segmenter("./qsegconf.ini", &segmenter);
    
    string line;
    ofstream output(arguments.index_file);
    while (getline(input, line)) {
        string btag, category, title;
        try {
            btag = regex_search(&record_regex, 1, line);
            title = regex_search(&record_regex, 2, line);
            category = regex_search(&category_regex, 1, btag);
        } catch (runtime_error &e) {
            cerr << e.what() << endl;
            cerr << "line: " << line << endl;
            continue;
        }
        
        vector<string> word_vec;
        retrieve_word(trie_tree, title, word_vec);
        if (word_vec.size() == 0)
            continue;

        vector<string> title_seg_vec;
        segment(segmenter, title, title_seg_vec);

        output << category << "\t";
        bool first_flag = true;
        for (vector<string>::const_iterator iter = word_vec.begin(); iter != word_vec.end(); ++iter) {
            bool is_seg = false;
            for (vector<string>::const_iterator seg_iter = title_seg_vec.begin(); seg_iter != title_seg_vec.end(); ++seg_iter)
                if (*iter == *seg_iter) {
                    is_seg = true;
                    break;
                }
            if (!is_seg)
                break;
            
            if (first_flag) {
                first_flag = false;
            } else {
                output << " ";
            }
            output << *iter;
        }
        output << endl;
    }

    regex_free(&category_regex);
    regex_free(&record_regex);

    return 0;
}

int retrieve_word(Container<Trie<String, int> > &trie_tree, const string &cand_str, vector<string> &retrieveWords)
{
    Range<uint8_t>ranges[MAX_CMD_WORD_NUM];
    int values[MAX_CMD_WORD_NUM];
    const char* cstr = cand_str.c_str();
    
    //matchAll
    int match_count = trie_tree[0].matchAll((uint8_t*)cstr, (uint8_t*)(cstr+ cand_str.length()), MAX_CMD_WORD_NUM, ranges, values, 1);
    uint8_t* pBegin = (uint8_t*)cstr;
    for(int i = 0; i < match_count; ++i)
    {
        string word = string(cand_str.substr(ranges[i].begin - pBegin, ranges[i].end - ranges[i].begin));
        retrieveWords.push_back(word);
    }
    if(retrieveWords.empty())
    {
        return 0;
    }
    return 0;
}

        
