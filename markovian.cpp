#include <iostream>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <random>
#include <fstream>
#include <regex>

using namespace std;

class Markovian {
public:
    int prev = -1;
    int pts_player = 0;
    int pts_cpu = 0;
    
    // game possibilities (1 = win & 0 = loss)
    unordered_map<int,vector<int>> possible;
    unordered_map<int,string> hands = {{1,"rock"},{2,"paper"},{3,"scissors"}};
    unordered_map<int,int> decision = {{0,1},{1,2},{2,0}};
    
    // Hidden Markov Model Transition Matrix
    vector<vector<int>> matrix_count = {{0,0,0},{0,0,0},{0,0,0}};
    vector<vector<double>> matrix_probability = {{0,0,0},{0,0,0},{0,0,0}};
    
    std::mutex mtx;
    
    Markovian() {
        HashMapSetup();
    }
    
    Markovian(string str) {
        HashMapSetup();
        read(str);
    }
    
    void HashMapSetup() {
        possible[0] = {-1,0,1};
        possible[1] = {1,-1,0};
        possible[2] = {0,1,-1};
    }
    
    void solveMatrix() {
        for (int i = 0; i < 3; i++) {
            int sum = 0;
            for (int j = 0; j < 3; j++) {
                sum += matrix_count[i][j];
            }
            if (sum != 0) {
                for (int j = 0; j < 3; j++) {
                    matrix_probability[i].at(j) = static_cast<double>(matrix_count[i][j]) / sum;
                }
            }
        }
    }
    
    double random() {
        std::mt19937_64 rng;
        // initialize the random number generator with time-dependent seed
        uint64_t timeSeed = std::chrono::high_resolution_clock::now().time_since_epoch().count();
        std::seed_seq ss{uint32_t(timeSeed & 0xffffffff), uint32_t(timeSeed>>32)};
        rng.seed(ss);
        // initialize a uniform distribution between 0 and 1
        std::uniform_real_distribution<double> unif(0, 1);
        // ready to generate random number
        double random = unif(rng);
        return random;
    }
    
    int randHand() {
        double r = random() * 3;
        if (r < 1) {
            cout << "Rock -> ";
            return 0;
        } else if (r >= 2) {
            cout << "Scissors -> ";
            return 2;
        } else {
            cout << "Paper -> ";
            return 1;
        }
    }
    
    int displayCpuMove(int x) {
        if (x == 0)
            cout << "Rock -> ";
        if (x == 1)
            cout << "Paper -> ";
        if (x == 2)
            cout << "Scissors -> ";
        return x;
    }
    
    int computerHand() {
        if (prev != -1) {
            int max = 0;
            for (int i = 1; i < 3; i++) {
                if (matrix_probability[prev][i] > matrix_probability[prev][max])
                    max = i;
            }
            return (matrix_probability[prev][max] > 0) ? decision[max] : randHand();
        } else {
            return randHand();
        }
    }
    
    void playHand(int x) {
        x -= 1;
        int ans;
        
        solveMatrix();
        ans = (prev != -1) ? possible[x][displayCpuMove(computerHand())] : possible[x][computerHand()];
        
        if (prev != -1) {
            matrix_count[prev].at(x) += 1;
        }
        
        prev = x;
        
        if (ans == 1) {
            pts_player++;
            cout << "win";
        }
        if (ans == 0) {
            pts_cpu++;
            cout << "loss";
        }
        if (ans == -1) {
            cout << "tie";
        }
        cout << " ... winratio = " << pts_player << ":" << pts_cpu << endl;
    }
    
    void read(string str) {
        matrix_count.clear();
        str.append(".csv");
        int i1, i2, i3;
        FILE *fp;
        fp = fopen(str.c_str(), "r");
        while (fscanf(fp, "%d,%d,%d\n", &i1, &i2, &i3) == 3) {
            matrix_count.push_back({i1,i2,i3});
        }
    }
    
    void save(string str) {
        str.append(".csv");
        std::ofstream ofs;
        ofs.open(str, std::ofstream::out | std::ofstream::trunc);
        for (int i = 0; i < 3; i++) {
            ofs << matrix_count[i][0] << "," << matrix_count[i][1] << "," << matrix_count[i][2] << endl;
        }
        ofs.close();
    }
};

int options() {
    int move;
    string str;
    
    cin >> str;
    if (isdigit(str[0]))
        move = str[0] - 48;
    
    if (move == 1) {
        cout << "\033[1A" << "\033[K" << "Rock (vs) ";
    } else if (move == 2) {
        cout << "\033[1A" << "\033[K" << "Paper (vs) ";
    } else if (move == 3) {
        cout << "\033[1A" << "\033[K" << "Scissors (vs) ";
    } else {
        cout << "\033[1A\033[K";
        return options();
    }
    return move;
}

inline bool exists(const std::string& name) {
    string filename = name;
    filename.append(".csv");
    ifstream f(filename.c_str());
    return f.good();
}

void read(string file) {
    std::ifstream f(file);

    if (f.is_open())
        std::cout << f.rdbuf();
}

string removeSpecialChars(string str) {
    const regex pattern("[^A-Za-z0-9]");
    return regex_replace(str, pattern, "");
}

int main(int argc, char** argv) {
    read("banner2.txt");
    
    if (argc == 2 && strcmp(argv[1], "help") == 0) {
        cout << "\033[3m";
        read("help.txt");
        cout << "\033[0m";
        return 1;
    } else if (argc < 3) {
        cout << "\033[3mUsage: markovian [user] [n]\n\n\033[0m";
        cerr << "\033[1;31merror:\033[1;0m missing a mandatory option. Try 'markovian help' for more info.\n";
        return 1;
    } else if (argc == 3) {
        cout << "Welcome to paper scissors \033[1;31mshoot!\033[1;0m\n\nEnter a move below to begin:\n";
    } else {
        cerr << "\033[1;31merror:\033[1;0m program takes 2 arguments (" << argc-1 << " supplied). Try 'markovian help' for more info.\n";
        return 1;
    }
    
    
    int n = atoi(argv[2]);
    
    // prevent OS command execution
    string name = removeSpecialChars(argv[1]);
    
    Markovian * game;
    
    if (exists(name)) {
        game = new Markovian(name);
    } else {
        game = new Markovian();
    }
    
    int cnt = 0;
    while (cnt < n) {
        int option = options();
        game->playHand(option);
        cnt++;
    }
    
    // saves markov transition matrix
    game->save(name);
    
    // epilogue
    if (game->pts_player > game->pts_cpu) {
        cout << "\n\033[3m\n\"History is Written by \033[1;32mVictors\033[1;0m.\"\n- Winston Churchill\n\033[0m";
    } else if (game->pts_player < game->pts_cpu) {
        cout << "\n\033[3m\"Robots will be able to do \033[1;31meverything\033[1;0m better than us.\033[0m\"\n- Elon Musk\n";
    } else {
        cout << "\n\033[3m\"Before God, we are all equally wise, and equally foolish.\"\033[0m\n- Albert Einstein\n";
    }
}
