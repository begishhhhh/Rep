#include <iostream>
#include <vector>
#include <random>
#include <cstdlib>
#include <windows.h> // для Sleep()

using namespace std;

class Renderer 
{
public:
    Renderer(int width, int height) : width_(width), height_(height) {}

    void render(const vector<vector<bool>>& field) 
    {
        clearScreen();

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                cout << (field[y][x] ? 'O' : ' ');
            }
            cout << endl;
        }
    }

private:
    void clearScreen() 
    {
        system("cls");
    }

private:
    int width_;
    int height_;
};

class Cell 
{
public:
    Cell(bool alive = false) : alive_(alive) {}

    bool isAlive() const { return alive_; }
    void setAlive(bool alive) { alive_ = alive; }

private:
    bool alive_;
};

class Field 
{
public:
    Field(int width, int height) : width_(width), height_(height) 
    {
        field_.resize(height_, vector<bool>(width_, false));
        // initializeRandomly();
        initializeFromInput();
    }

    int getWidth() const { return width_; }
    int getHeight() const { return height_; }

    bool getCell(int x, int y) const 
    {
        x = (x + width_) % width_;
        y = (y + height_) % height_;
        return field_[y][x];
    }

    void setCell(int x, int y, bool alive) {
        field_[y][x] = alive;
    }

    const vector<vector<bool>>& getFieldData() const {
        return field_;
    }

private:
    void initializeRandomly() 
    {
        random_device rd;
        mt19937 gen(rd());
        uniform_real_distribution<> dis(0.0, 1.0);

        for (int y = 0; y < height_; ++y) {
            for (int x = 0; x < width_; ++x) {
                field_[y][x] = (dis(gen) < 0.5);
            }
        }
    }
    void initializeFromInput() 
    {
        cout << "Enter the coordinates, enter '-1 -1' to finish:" << endl;
        int x, y;
        while (true) 
        {
            cin >> x >> y;
            if (x == -1 && y == -1) {
                break;
            }
            if (x >= 0 && x < width_ && y >= 0 && y < height_) {
                field_[y][x] = true;
            } 
            else {
                cout << "Invalid coordinates" << endl;
            }
        }
    }

private:
    int width_;
    int height_;
    vector<vector<bool>> field_;
};

class Game 
{
public:
    Game(int width, int height) : field_(width, height), renderer_(width, height) {}

    void run(int numGenerations) 
    {
        for (int generation = 0; generation < numGenerations; ++generation) 
        {
            renderer_.render(field_.getFieldData());
            updateField();
            Sleep(500);
        }
    }

private:
    void updateField() 
    {
        vector<vector<bool>> nextField = field_.getFieldData();

        for (int y = 0; y < field_.getHeight(); ++y) {
            for (int x = 0; x < field_.getWidth(); ++x) {
                int liveNeighbors = countLiveNeighbors(x, y);

                if (field_.getCell(x, y)) {
                    if (liveNeighbors < 2 || liveNeighbors > 3) {
                        nextField[y][x] = false;
                    }
                } 
                else {
                    if (liveNeighbors == 3) {
                        nextField[y][x] = true;
                    }
                }
            }
        }

        for (int y = 0; y < field_.getHeight(); ++y) {
            for (int x = 0; x < field_.getWidth(); ++x) {
                field_.setCell(x, y, nextField[y][x]);
            }
        }
    }

    int countLiveNeighbors(int x, int y) const 
    {
        int count = 0;
        for (int i = -1; i <= 1; ++i) {
            for (int j = -1; j <= 1; ++j) {
                if (i == 0 && j == 0) continue;
                if (field_.getCell(x + j, y + i)) {
                    count++;
                }
            }
        }
        return count;
    }


private:
    Field field_;
    Renderer renderer_;
};

int main() 
{
    int width = 110;
    int height = 30;
    int numGenerations = 50;

    Game game(width, height);
    game.run(numGenerations);

    return 0;
}
