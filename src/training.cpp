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
#define likely(x)       __builtin_expect((x),1)
#define unlikely(x)     __builtin_expect((x),0)

#ifdef _OPENMP
  #include <omp.h>
#else
  #define omp_get_thread_num() 0
#endif


string basic_pool[] = { "[a-z]", "[A-Z]", "[0-9]", "\\?" "\t", ".", "\\{", "\\}",
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
  cerr << "Building initial pool" << endl;
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=0; i < p; i++){
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
  }
}


void genetic_operations(vector<Regex> &pool, int n, double epsilon){
  int r;
  for (int i=0; i < n; i++){
    r = rand() % 100;
    if (unlikely(r < epsilon*100)){
      pool.push_back(mutation(pool[rand() % pool.size()], pool));
    } else {
      pool.push_back(crossover(pool[rand() % pool.size()], pool[rand() % pool.size()]));
    }
  }
}


long int count_matches(const Regex &regex, vector<ifstream> &files){
  long int matches = 0, priv_matches = 0;
  string text;
  regex_iterator<string::iterator> rend;
  std::regex e;

  #pragma omp parallel private(text, e, priv_matches)
  {
    #pragma omp for
    for (int i=0; i < files.size(); i++){
      priv_matches = 0;
      text = "";
      while(files[i].peek() != EOF)
        text += files[i].get();
      files[i].seekg(0, files[i].beg);

      try {
        e.assign(regex.toString());
        regex_iterator<string::iterator> rit(text.begin(), text.end(), e);
        while (rit!=rend){
          priv_matches++;
          ++rit;
        }
        cerr << i;
      } catch (...) {
        matches = 0;
      }
    }
    #pragma omp atomic
      matches += priv_matches;
  }

  return matches;
}

long int count_chars(vector<ifstream> &files){
  long int count = 0;

  for (vector<ifstream>::iterator it = files.begin(); it != files.end(); ++it){
    it->seekg(0, it->end);
    count += it->tellg();
    it->seekg(0, it->beg);
  }

  return count;
}


struct Cmp {
  bool operator() (const pair<Regex*, double>& lpair, const pair<Regex*, double>& rpair) const{
    return lpair.second > rpair.second;
  }
};

vector<double> select_fittest(vector<Regex> &pool, int k, vector<ifstream> &current_format_files, vector<ifstream> &other_formats_files){
  long int current_format_chars_count = count_chars(current_format_files);
  long int other_formats_chars_count = count_chars(other_formats_files);
  set<pair<Regex*, double>, Cmp> regex_goodness_set;
  cerr << "Selecting fittest 1" << endl;
  double current_matches_mean, other_matches_mean;
  for (int i=0; i < pool.size(); i++){
    pair<Regex*, double> p;
    p.first = &(pool[i]);
    current_matches_mean = (long double)count_matches(pool[i], current_format_files) / (long double)current_format_chars_count;
    other_matches_mean = (long double)count_matches(pool[i], other_formats_files) / (long double)other_formats_chars_count;
    p.second = current_matches_mean / (1000*other_matches_mean + 1);
    regex_goodness_set.insert(p);
  }

  cerr << "Selecting fittest 2" << endl;
  vector<double> goodness;
  vector<Regex> new_pool;
  int i = 0;
  for (set<std::pair<Regex*, double>, Cmp>::iterator it = regex_goodness_set.begin(); i < k && it != regex_goodness_set.end(); i++, ++it){
    new_pool.push_back(*(it->first));
    goodness.push_back(it->second);
    cerr << *(it->first) << " (" << it->second << ")" << endl;
  }
  cerr << endl <<  "------------------------------------" << endl << endl;
  pool = new_pool;

  cerr << "Selecting fittest 3" << endl;
  return goodness;
}


void complete_pool(vector<Regex> &pool, int p, double epsilon, vector<ifstream> &files){
  cerr << "genetic operations"  << endl;
  genetic_operations(pool, (int)(1.0/3 * (p - pool.size())), epsilon);

  cerr << "completting pool" << endl;
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=pool.size(); i < p; i++)
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
}


void training(const fs::path &current_format_path, const fs::path &root_path, OutputTemplate &output_template, int n, int p, int k, int k_0, double epsilon){
  vector<ifstream> current_format_streams;
  vector<ifstream> other_formats_streams;
  fs::directory_iterator end_it;

  for(fs::directory_iterator it(root_path/current_format_path); it != end_it; ++it)
    if (likely(fs::is_regular_file(it->status()))){
      current_format_streams.push_back(ifstream(fs::system_complete(*it).string()));
    }

  for(fs::directory_iterator it(root_path); it != end_it; ++it)
    if (likely(fs::is_directory(it->status())) && *it != current_format_path)
      for(fs::directory_iterator dir_it(*it); dir_it != end_it; ++dir_it)
        if (likely(fs::is_regular_file(dir_it->status()))){
          other_formats_streams.push_back(ifstream(fs::system_complete(*dir_it).string()));
        }


  cout << "Entrenando expresiones " << current_format_path.string() << " (0%)" << flush;

  vector<Regex> pool;
  buildInitialPool(pool, current_format_streams, p);
  for (int i=0; i < n; i++){
    complete_pool(pool, p, epsilon, current_format_streams);
    select_fittest(pool, k, current_format_streams, other_formats_streams);
    cout << "\rEntrenando expresiones " << current_format_path.string() << " (" << 100*i/n << "%)" << flush;
  }
  vector<double> goodness = select_fittest(pool, k_0, current_format_streams, other_formats_streams);
  cout << "\rEntrenando expresiones " << current_format_path.string() << " (100%)" << flush << endl;

  for (int i=0; i < pool.size(); i++)
    output_template.addRegex(current_format_path.string(), pool[i].toString(), goodness[i]);
}


int main(int argc, char** argv){
  int p = 50, k, k_0, iter = 1000;
  double epsilon = 0.01, k_proportion = 1.0/3, k_0_proportion = 1.0/5;
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
      k_proportion = strtod(argv[i+1], NULL);
      i+=2;
    } else if (strcmp(argv[i], "-k_0") == 0){
      k_0_proportion = strtod(argv[i+1], NULL);
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

  k = (int)(k_proportion * p);
  k_0 = (int)(k_0_proportion * p);

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
