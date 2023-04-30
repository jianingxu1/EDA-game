#include <iostream>
#include <stdlib.h>
#include <cstring>
#include <fstream>
#include <sstream> 
#include <string>
#include <vector>
#include <thread>
#include <vector>
#include <queue>
using namespace std;



int avgPoints[4] = {0, 0, 0, 0};
vector< vector<int> > wins(4, vector<int>(4, 0));
string player[4];

void threadf(int seed){
    //Executar commanda
    string c = "./Game " + player[0] + " " + player[1] + " " + player[2] + " " + player[3] + " -s " + to_string(seed) + " < default.cnf > " + to_string(seed) + ".res 2> /dev/null";
    char char_array[c.length()+1];
    strcpy(char_array, c.c_str());
    system(char_array);
    
    //Llegir resultat
    ifstream f = ifstream(to_string(seed)+".res");
    string temp;

    while(getline(f, temp)){
        if (temp.find("round 200") != string::npos) break;
    }
    getline(f, temp); //Llegim espai en blanc
    getline(f, temp); //Llegim score
    
    stringstream X(temp);
    string part;
    char tab = 9;
    getline(X, part, tab);
    priority_queue<pair<int, int>, vector< pair<int, int> >, less< pair<int, int> > > results;
    for (int i = 0; i < 4; ++i) {
        getline(X, part, tab);
        int points = stoi(part);
        results.push({points, i});
        avgPoints[i] += points;
    }
    int i = 0;
    while (not results.empty()) {
        ++wins[results.top().second][i];
        results.pop();
        ++i;
    }
}

int main(){
    cout << "List of players available: " << endl;
    system("./Game -l");
    
    int n,seed;
    cout << "Name of the players:" << endl;
    for (int i = 0; i < 4; ++i) cin >> player[i];
    cout << "Number of rounds:" << endl;
    cin >> n;
    cout << "Initial seed (the next seeds will be seed + 1, seed + 2, ..., seed + n - 1):" << endl;
    cin >> seed;
    cout << "Running the games..." << endl;

    // Process games
    vector<thread> threads = vector<thread>(0);
    for(int i=0; i<n; ++i) {
        thread t(threadf, i+seed);
        threads.push_back(move(t));
    }

    
    for(int i=0; i<n; ++i) {
        threads[i].join();
    }
    
    for(int i=0; i<n; ++i){
        string c = "rm " + to_string(i+seed) + ".res";
        char char_array[c.length()+1];
        strcpy(char_array, c.c_str());
        system(char_array);
    }
    
    // Print results
    cout << "Initial seed: " << seed << endl;
    cout << "Number of rounds: " << n << endl;
    for (int i = 0; i < 4; ++i) {
        cout << player[i] << ":\t";
        if (player[i].length() < 7) cout << "\t";
        cout << "1st: " << wins[i][0] << "\t2nd: " << wins[i][1] << "\t3rd: " << wins[i][2] << "\t4th: " << wins[i][3] << "\tAvg: " << avgPoints[i]/n << endl;
    }
}
