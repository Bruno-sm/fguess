#include <iostream>
#include <string>
#include <vector>
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
                        "\\n", "\\", ":", "\\<", "\\>", "#", "%", "~", "@", "=", "\\*", " ",
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


void buildInitialPool(vector<Regex> &pool, const fs::path &files_path, int p){
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=0; i < basic_size; i++){
    pool.push_back(Regex(basic_pool[i]));
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


vector<int> select_fittest(vector<Regex> &pool, int k, const fs::path &current_format_path, const fs::path &root_path){
  int t = pool.size() - k;
  for (int i=0; i < t; i++)
    pool.pop_back();
  vector<int> goodness;
  for (int i=0; i < k; i++)
    goodness.push_back(i);
  return goodness;
  // Eliminar exprsiones repetidas
}


void complete_pool(vector<Regex> &pool, int p, const fs::path &files_path){
  int basic_size = sizeof(basic_pool)/sizeof(string);
  for (int i=pool.size(); i < p; i++)
    pool.push_back(Regex(basic_pool[rand() % basic_size]));
}


void training(const fs::path &current_format_path, const fs::path &root_path, OutputTemplate &output_template, int n, int p, int k, int k_0, double epsilon){
  cout << "Entrenando expresiones " << current_format_path.string() << " (0%)" << flush;
  vector<Regex> pool;
  buildInitialPool(pool, current_format_path, p);
  for (int i=0; i < n; i++){
    genetic_operations(pool, epsilon);
    select_fittest(pool, k, current_format_path, root_path); // TODO abrir los archivos en dos vectores y pasarlos en vez de los paths
    complete_pool(pool, p, current_format_path);
    cout << "\rEntrenando expresiones " << current_format_path.string() << " (" << 100*i/n << "%)" << flush;
  }
  vector<int> goodness = select_fittest(pool, k_0, current_format_path, root_path);
  cout << "\rEntrenando expresiones " << current_format_path.string() << " (100%)" << flush << endl;

  for (int i=0; i < pool.size(); i++)
    output_template.addRegex(current_format_path.string(), pool[i].toString(), goodness[i]);
}


int main(int argc, char** argv){
  int p = 20, k = 5, k_0 = 15, iter = 1000;
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
