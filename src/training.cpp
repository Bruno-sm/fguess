#include <iostream>
#include <string>
#include <vector>
#include <set>
#include <utility>
#include <regex>
#include <map>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include <boost/filesystem.hpp>
#include "regex.hpp"
#include "file_templates.hpp"
using namespace std;
namespace fs = boost::filesystem;


string basic_pool[] = { "[a-z]", "[A-Z]", "[0-9]", "\\?" "\t", ".",
                        "\\n", ":", "\\<", "\\>", "#", "%", "~", "@", "=", "\\*",
                        "\\+", "\\-", "0", "1", "2", "3", "4", "5", "6", "7", "8", "9",
                        "a", "A", "b", "B", "c", "C", "d", "D", "e", "E", "f", "F", "g",
                        "G", "h", "H", "i", "I", "j", "J", "k", "K", "l", "L", "m", "M",
                        "n", "N", "o", "O", "p", "P", "q", "Q", "r", "R", "s", "S", "t",
                        "T", "u", "U", "v", "V", "w", "W", "x", "X", "y", "Y", "z", "Z"};


Regex crossover(const Regex &regex1, const Regex &regex2){
  int operation = (int)rand() % 4;
  switch (operation) {
    case 0: return regex1 * regex2; break;
    case 1: return regex1 | regex2; break;
    case 2: return regex1++; break;
    case 3: return *regex1; break;
  }
}


Regex mutation(const Regex &regex, const vector<Regex> &pool){
  Regex rand_word = pool[rand() % pool.size()];
  int split_point = (int)(rand()%(regex.length()));
  return regex.head(split_point) + rand_word + regex.tail(regex.length()-split_point);
}


void buildInitialPool(vector<Regex> &pool, vector<ifstream> &files, int p){
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=0; i < basic_size; i++){
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
  }

  int file_index;
  int char_position;
  char character;
  string str;
  for (int i=basic_size; i < p; i++){
    file_index = (int)(rand() % files.size());
    char_position = (int)(rand() % files[file_index].gcount());
    files[file_index].seekg(char_position, files[file_index].beg);
    while (!isspace(files[file_index].peek()) && files[file_index].peek() != EOF){
      files[file_index].get(character);
      str.push_back(character);
    }
    pool.push_back(Regex(str));
    str = "";
    files[file_index].seekg(0, files[file_index].beg);
  }
}


void genetic_operations(vector<Regex> &pool, double epsilon){
  // Dos tercios de la piscina serán expresiones derivadas de las existentes
  int p = pool.size();
  for (int i=0; i < (int)2*p; i++){
    int r = rand() % (int)(epsilon*100);
    switch (r) {
      case 0: pool.push_back(mutation(pool[rand() % pool.size()], pool)); break;
      default: pool.push_back(crossover(pool[rand() % pool.size()], pool[rand() % pool.size()]));
    }
  }
}


int count_matches(const Regex &regex, vector<ifstream> &files){
  int matches = 0;
  string text;

  for (vector<ifstream>::iterator it = files.begin(); it != files.end(); ++it){
    text = "";
    while(it->peek() != EOF)
      text += it->get();
    it->seekg(0, it->beg);

    std::regex r(regex.toString());
    auto r_begin = std::sregex_iterator(
        text.begin(), text.end(), r);
    auto r_end = std::sregex_iterator();

    matches += std::distance(r_begin, r_end);
  }

  return matches;
}

/*
vector<int> count_matches(const vector<Regex> &regexs, const vector<fs::path> files_paths){
  CountLexTemplate countTemplate;
  for (vector<Regex>::const_iterator it = regexs.begin(); it != regexs.end(); ++it)
    countTemplate.addRegex(it->toString());
  countTemplate.save("count.lex");

  system("echo "" > out.txt");
  system("flex count.lex");
  system("gcc lex.yy.c -o count -lfl");
  fs::directory_iterator end_it;
  string command_str = "./count ";
  for (vector<fs::path>::const_iterator it = files_paths.begin(); it != files_paths.end(); ++it){
    command_str += it->string();
    command_str += " ";
  }
  command_str += "> out.txt";
  system(command_str.c_str());
  ifstream ifs("out.txt");
  vector<int> matches;
  int num;
  for (int i=0; i < regexs.size(); i++){
    ifs >> num;
    matches.push_back(num);
  }

  return matches;
}*/


int count_chars(vector<ifstream> &files){
  int count = 0;

  for (vector<ifstream>::iterator it = files.begin(); it != files.end(); ++it){
    it->seekg(0, it->end);
    count += it->tellg();
    it->seekg(0, it->beg);
  }

  return count;
}


struct Cmp {
  bool operator() (const pair<Regex*, double>& lpair, const pair<Regex*, double>& rpair) const{
    return lpair.second >= rpair.second && !(*(lpair.first) == *(rpair.first));
  }
};

vector<int> select_fittest(vector<Regex> &pool, int k, vector<ifstream> &current_format_files, vector<ifstream> &other_formats_files){
  int current_format_chars_count = count_chars(current_format_files);
  int other_formats_chars_count = count_chars(other_formats_files);
  set<pair<Regex*, double>, Cmp> regex_goodness_set;

  double current_matches_mean, other_matches_mean;
  for (int i=0; i < pool.size(); i++){
    pair<Regex*, double> p;
    p.first = &(pool[i]);
    current_matches_mean = (double)count_matches(pool[i], current_format_files) / (double)current_format_chars_count;
    other_matches_mean = (double)count_matches(pool[i], other_formats_files) / (double)other_formats_chars_count;
    p.second = current_matches_mean / (1000*other_matches_mean + 1);
    regex_goodness_set.insert(p);
  }

  vector<int> goodness;
  vector<Regex> new_pool;
  int i = 0;
  for (set<std::pair<Regex*, double>, Cmp>::iterator it = regex_goodness_set.begin(); i < k && it != regex_goodness_set.end(); i++, ++it){
    new_pool.push_back(*(it->first));
    goodness.push_back(it->second);
    cerr << *(it->first) << " (" << it->second << ")" << endl;
  }
  cerr << endl <<  "------------------------------------" << endl << endl;
  pool = new_pool;

  return goodness;
}


void complete_pool(vector<Regex> &pool, int p, const vector<ifstream> &files){
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=pool.size(); i < p; i++)
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
}


void training(const fs::path &current_format_path, const fs::path &root_path, OutputTemplate &output_template, int n, int p, int k, int k_0, double epsilon){
  vector<fs::path> current_format_file_paths;
  vector<fs::path> other_formats_file_paths;
  vector<ifstream> current_format_streams;
  vector<ifstream> other_formats_streams;
  fs::directory_iterator end_it;

  for(fs::directory_iterator it(root_path/current_format_path); it != end_it; ++it)
    if (fs::is_regular_file(it->status())){
      current_format_file_paths.push_back(fs::system_complete(*it));
      current_format_streams.push_back(ifstream(fs::system_complete(*it).string()));
    }

  for(fs::directory_iterator it(root_path); it != end_it; ++it)
    if (fs::is_directory(it->status()) && *it != current_format_path)
      for(fs::directory_iterator dir_it(*it); dir_it != end_it; ++dir_it)
        if (fs::is_regular_file(dir_it->status())){
          other_formats_file_paths.push_back(fs::system_complete(*dir_it));
          other_formats_streams.push_back(ifstream(fs::system_complete(*dir_it).string()));
        }


  cout << "Entrenando expresiones " << current_format_path.string() << " (0%)" << flush;

  vector<Regex> pool;
  buildInitialPool(pool, current_format_streams, p);
  for (int i=0; i < n; i++){
    genetic_operations(pool, epsilon);
    select_fittest(pool, k, current_format_streams, other_formats_streams);
    complete_pool(pool, p, current_format_streams);
    cout << "\rEntrenando expresiones " << current_format_path.string() << " (" << 100*i/n << "%)" << flush;
  }
  vector<int> goodness = select_fittest(pool, k_0, current_format_streams, other_formats_streams);
  cout << "\rEntrenando expresiones " << current_format_path.string() << " (100%)" << flush << endl;

  for (int i=0; i < pool.size(); i++)
    output_template.addRegex(current_format_path.string(), pool[i].toString(), goodness[i]);
}


int main(int argc, char** argv){
  int p = 50, k = 30, k_0 = 10, iter = 1000;
  double epsilon = 0.01;
  fs::path examples_path(fs::initial_path<fs::path>());

  if (argc == 1){
    cerr << "Especifica el directorio con los archivos de ejemplo" << endl;
    return -1;
  }
  examples_path = fs::system_complete(fs::path(argv[1]));

  for (int i=2; i < argc;)
    if (strcmp(argv[i], "-p") == 0){
      p = atoi(argv[i+1]);
      i+=2;
    } else if (strcmp(argv[i], "-k") == 0){
      k = atoi(argv[i+1]);
      i+=2;
    } else if (strcmp(argv[i], "-k_0") == 0){
      k_0 = atoi(argv[i+1]);
      i+=2;
    } else if (strcmp(argv[i], "-epsilon") == 0){
      epsilon = strtod(argv[i+1], NULL);
      i+=2;
    } else if (strcmp(argv[i], "-iter") == 0){
      iter = atoi(argv[i+1]);
      i+=2;
    } else {
      i++;
    }

  time_t t;
  srand((unsigned) time(&t));

  if (!fs::exists(examples_path)) {
    cerr << "El directorio especificado no existe" << endl;
    return -1;
  }

  vector<fs::path> example_folders;
  fs::directory_iterator end_it;
  for(fs::directory_iterator it(examples_path); it != end_it; ++it){
    if (fs::is_directory(it->status()))
      example_folders.push_back(it->path().filename());
  }

  int count;
  cout << "Muestras encontradas:" << endl;
  for (vector<fs::path>::iterator it=example_folders.begin(); it!=example_folders.end(); ++it){
    cout << it->string();
    count = 0;
    for (fs::directory_iterator dir_it(examples_path/(*it)); dir_it != end_it; ++dir_it)
      if (fs::is_regular_file(dir_it->status()))
        count++;
    cout << " [" << count << "]" << endl;
  }

  string res;
  cout << "¿Comenzar el entrenamiento? (s/n) ";
  cin >> res;
  if (res != "s" && res != "S")
    return 0;
  cout << "Empezando a entrenar:" << endl;

  OutputTemplate fguess_template;
  for (vector<fs::path>::iterator it=example_folders.begin(); it!=example_folders.end(); ++it)
    training(*it, examples_path, fguess_template, iter, p, k, k_0, epsilon);

  fguess_template.save("fguess.lex");

  return 0;
}
