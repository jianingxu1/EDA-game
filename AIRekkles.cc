#include "Player.hh"
/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME Rekkles

struct PLAYER_NAME : public Player {

  /**
   * Factory: returns a new instance of this class.
   * Do not modify this function.
   */
  static Player* factory () {
    return new PLAYER_NAME;
  }

  /**
   * Types and attributes for your player can be defined here.
   */
  enum CellContent {
    WASTE,               // Waste is in the cell
    DEAD,                // A dead unit is in the cell
    FOOD,                // Food is in the cell
    ZOMBIE,              // A zombie is in the cell
    ENEMY,               // An enemy is in the cell
    UNIT,                // A friendly unit is in the cell
    EMPTYOWNED,          // Cell is empty and owned by me
    EMPTYNOTOWNED,       // Cell is empty and not owned by me (owned by another player or not owned by anyone)
  };
  
  typedef vector<int> VI;
  typedef vector<VI> VVI;

  typedef vector<bool> VB;
  typedef vector<VB> VVB;

  typedef pair<int, int> P;
  typedef vector<P> VP;
  typedef vector<VP> VVP;

  typedef vector<CellContent> VC;
  typedef vector<VC> VVC;

  const int n = 60;
  int max_bfs;
  vector<Dir> dirs = {Up, Down, Left, Right};

  VI myUnits;                   // Vector that contains my units
  set<int> setMyUnits;         // Set that contains my units (from units)
  VVC board = VVC(n, VC(n));   // Matrix that contains all cells content

  /**
   * Play method, invoked once per each round.
   */
  CellContent cellToCellContent(const Cell& cell) {
    if (cell.type == Waste) return WASTE;
    if (cell.food) return FOOD;
    // An empty cell
    if (cell.id == -1) {
      if (cell.owner != me()) return EMPTYNOTOWNED;
      return EMPTYOWNED; 
    }
    Unit u = unit(cell.id);
    if (u.type == Zombie) return ZOMBIE;
    if (u.type == Dead) return DEAD;
    if (u.player == me()) return UNIT;
    return ENEMY;
  }

  void getBoardContent() {
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        board[i][j] = cellToCellContent(cell(i, j));
      }
    }
  }
  
  void getMyUnits() {
    setMyUnits.clear();
    myUnits = alive_units(me());
    for (auto id : myUnits) setMyUnits.insert(id);
  }

  void getRoundData() {
    getBoardContent();
    getMyUnits();
  }


  // Returns true if the position <p> is inside map and is not Waste
  bool posOk(const Pos& p) {
    return (pos_ok(p) and board[p.i][p.j] != WASTE);
  }

  // Returns true if there is a dead unit in position <p>
  bool isPosDead(const Pos& p) {
    return board[p.i][p.j] == DEAD;
  }


  struct TargetPosition {
    Pos p;
    Dir d;
    int dist;
  };

  struct PositionValue{
    TargetPosition tp;
    int value;
  };

  struct Compare {
    bool operator() (const PositionValue& a, const PositionValue& b) {
      if (a.value != b.value) return a.value < b.value;
      return a.tp.dist >= b.tp.dist;
    }
  };
  
  int calculatePriority(CellContent content, int targetDist) {
    if (content == ZOMBIE) {
      if (targetDist == 1) return 23 - targetDist;
      else return 5 - targetDist;
    }
    else if (content == FOOD) {
      return 23 - targetDist;
    }
    else if (content == DEAD) {
      return 12 - targetDist;  // change to 23?
    }
    return 0;
  }
  // Returns Dir and Pos to the closest Food, Zombie or Enemy.
  // If it does not find any, returns Dir and Pos of first available cell or first empty not owned cell.
  TargetPosition findClosestUnit(const Unit& u, const VVI& distances, VVB& visited, queue<TargetPosition>& q) {
    if (myUnits.size() <= 40) max_bfs = 50;
    else if (myUnits.size() <= 60) max_bfs = 30;
    else max_bfs = 20;
    if (max_bfs > num_rounds() - round()) max_bfs = num_rounds() - round();
    TargetPosition firstEmpty = q.front();
    bool found = false;

    int str = strength(me());
    bool isInfected = u.rounds_for_zombie != -1;
    int stepsPossibleAsZombie = u.rounds_for_zombie;
    if (isInfected) max_bfs = stepsPossibleAsZombie*2.5 + 1;
    priority_queue<PositionValue, vector<PositionValue>, Compare> targets;
    // If we don't find anything, go to the first available cell or first empty not owned cell
    while (not q.empty() and max_bfs >= q.front().dist) {
      TargetPosition target = q.front();
      Pos p = target.p;
      Dir d = target.d;
      int dist = target.dist;
      q.pop();
      if (visited[p.i][p.j]) continue;
      visited[p.i][p.j] = true;

      bool isTargeted = distances[p.i][p.j] != -1;
      int prevDist = distances[p.i][p.j];
      // If continue, treat it as a wall and stop searching there
      // If break, do not go there but continue searching
      // Else, put into priority queue
      CellContent content = board[p.i][p.j];
      int priority;
      switch(content) {
        case WASTE: continue;
        case ENEMY:
          if (isInfected and dist > 2*stepsPossibleAsZombie) continue;
          if (isTargeted) {
            if (dist >= prevDist) break;
            else if (prevDist == 2 and myUnits.size() >= 15) break;
          }
          if (true) {
            int enemyStr = strength(unit(cell(p).id).player);
            double probability = double(str)/(double(str)+double(enemyStr));
            double totalProbability = probability + 0.3;
            if (probability >= 0.75) targets.push({target, 16-dist});
            else if (probability >= 0.63) targets.push({target, 13-dist});
            else if (probability >= 0.5) targets.push({target, 10-dist});
            else if (probability >= 0.286) targets.push({target, 4-dist});
            else continue;
          }
          break;
        case ZOMBIE:
          if (isInfected and dist > 2.5*stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          if (dist == 1 or dist == 2) return target;
          priority = calculatePriority(content, dist);
          targets.push({target, priority});
          break;
        case FOOD:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          priority = calculatePriority(content, dist);
          targets.push({target, priority});
          break;
        case DEAD:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          // If we arrive 1 step early, just at time or later: GOs
          if (dist < unit(cell({p.i, p.j}).id).rounds_for_zombie) {
            max_bfs = 1.4*unit(cell({p.i, p.j}).id).rounds_for_zombie;
            continue;
          }
          priority = calculatePriority(content, dist);
          targets.push({target, priority});
          break;
        case EMPTYNOTOWNED:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          // Find first not owned empty cell
          if (not found) {
            firstEmpty = target;
            found = true;
          }
          break;
        default: break;
      }
      // Check all four directions in a random order
      random_shuffle(dirs.begin(), dirs.end());
      for (auto dir : dirs) {
        Pos newPos = p + dir;
        TargetPosition auxTarget;
        auxTarget.p = newPos;
        auxTarget.d = d;
        auxTarget.dist = dist + 1;
        if (pos_ok(newPos) and not visited[newPos.i][newPos.j]) q.push(auxTarget);
      }
    }
    // Return most priority target
    if (targets.size() >= 1) return targets.top().tp;
    return firstEmpty;
  }

  // Mark position as Targeted and return the direction to go there
  TargetPosition findNextMove(const Unit& u, const VVI& distances) {
    VVB visited(n, VB(n, false));
    queue<TargetPosition> q;
    visited[u.pos.i][u.pos.j] = true;
    // Initial movements
    random_shuffle(dirs.begin(), dirs.end());
    for (auto d : dirs) {
      Pos newPos = u.pos + d;
      if (posOk(newPos)) {
        TargetPosition target;
        target.p = newPos;
        target.dist = 1;
        target.d = d;
        // If there's an enemy next to us, prioritize movement to kill him.
        if (board[newPos.i][newPos.j] == ENEMY and distances[newPos.i][newPos.j] == -1) return target;
        q.push(target);
      }
    }
    // If only one direction is possible, go there
    if (q.size() == 1) return q.front();
    return findClosestUnit(u, distances, visited, q);
  }
  
  Dir getRandomDir(vector<Dir>& possDirs) {
    if (possDirs.size() == 1) return possDirs[0];
    random_shuffle(possDirs.begin(), possDirs.end());
    return possDirs[0];
  }

  // Returns true if there is a zombie close to position p (at distance 1 horizontally, vertically or diagonally)
  bool isZombieClose(const Pos& p, Pos& zombiePos) {
    for (int i = p.i - 1; i <= p.i + 1; ++i) {
      for (int j = p.j - 1; j <= p.j + 1; ++j) {
        if (pos_ok(i, j) and board[i][j] == ZOMBIE) {
          zombiePos.i = i;
          zombiePos.j = j;
          return true;
        }
      }
    }
    return false;
  }  

  Dir zombieBestMove(const Unit& u, const Pos& zombiePos, int dist, Dir d) {
    if (dist == 1) return d;
    Pos unitPos = u.pos;
    int a = zombiePos.i - unitPos.i;
    int b = zombiePos.j- unitPos.j;
    if (dist == 2 and u.rounds_for_zombie == -1) {
      vector<Dir> possDirs;
      // Zombie in a straight line case
      if (a == 0) {
        if (posOk(unitPos + Up) and not isPosDead(unitPos + Up)) possDirs.push_back(Up);
        if (posOk(unitPos + Down) and not isPosDead(unitPos + Down)) possDirs.push_back(Down);
        if (possDirs.size() != 0) return getRandomDir(possDirs);    // RANDOM: Get either of them. IMPROVE -> BFS and make a thoughtful decision
        // Do not move
        return DR;
      }
      if (b == 0) {
        if (posOk(unitPos + Right) and not isPosDead(unitPos + Right)) possDirs.push_back(Right);
        if (posOk(unitPos + Left) and not isPosDead(unitPos + Left)) possDirs.push_back(Left);
        if (possDirs.size() != 0) return getRandomDir(possDirs);    // RANDOM: Get either of them. IMPROVE -> BFS and make a thoughtful decision
        // Do not move
        return DR;
      }
      // Zombie in a diagonal case
      if (a == 1 and posOk(unitPos + Up) and not isPosDead(unitPos + Up)) possDirs.push_back(Up);
      else if (a == -1 and posOk(unitPos + Down) and not isPosDead(unitPos + Down)) possDirs.push_back(Down);
      if (b == 1 and posOk(unitPos + Left) and not isPosDead(unitPos + Left)) possDirs.push_back(Left);
      else if (b == -1 and posOk(unitPos + Right) and not isPosDead(unitPos + Right)) possDirs.push_back(Right);
      if (possDirs.size() != 0) return getRandomDir(possDirs);    // RANDOM: Get either of them. IMPROVE -> BFS and make a thoughtful decision
      return d;
    }
    if (dist == 3 and ((abs(a) == 1 and abs(b) == 2) or (abs(a) == 2 and abs(b) == 1))) {
      if (a == -1 and posOk(unitPos + Up)) return Up;
      if (a == 1 and posOk(unitPos + Down)) return Down;
      if (b == -1 and posOk(unitPos + Right)) return Right;
      if (b == 1 and posOk(unitPos + Left)) return Left;
      // Do not move
      return DR;
    }
    // General case: Go to greatest x or y position. If cannot, go to d.
    if (abs(a) > abs(b)) {
      if (a > 0 and posOk(unitPos + Down) and not isPosDead(unitPos + Down)) return Down;
      if (a < 0 and posOk(unitPos + Up) and not isPosDead(unitPos + Up)) return Up;
    }
    else if (abs(a) < abs(b)) {
      if (b > 0 and posOk(unitPos + Right) and not isPosDead(unitPos + Right)) return Right;
      if (b < 0 and posOk(unitPos + Left) and not isPosDead(unitPos + Left)) return Left;
    }
    return d;
  }

  struct Command {
    int id;
    Dir direction;
    int priority; // Lower value, more priority. Higher value, less priority.
  };

  struct CompareCommand {
    bool operator() (const Command& a, const Command& b) {
      return a.priority >= b.priority;  // Lower value, more priority. Higher value, less priority.
    }
  };

  struct Movement {
    int priority;   // Lower value, more priority. Higher value, less priority.
    Dir direction;
  };

  void getCommands(const map<int, Movement>& movements, priority_queue<Command, vector<Command>, CompareCommand>& commands) {
    for (auto mov : movements) {
      Command c;
      c.id = mov.first;
      c.direction = mov.second.direction;
      c.priority = mov.second.priority;
      commands.push(c);
    }
  }

  void executeCommands(priority_queue<Command, vector<Command>, CompareCommand>& commands) {
    while(not commands.empty()) {
      Command c = commands.top();
      move(c.id, c.direction);
      commands.pop();
    }
  }

  void getMovements(map<int, Movement>& movements) {
    // Contains the id of the unit planning to go to that position
    // If value is -1, no unit plans to go to that position
    // Else, there is a unit planning to go to that position and the value is the steps it has to take to get there
    VVI ids = VVI(n, VI(n, -1));
    VVI distances = VVI(n, VI(n, -1));
    while (not setMyUnits.empty()) {
      auto it = setMyUnits.begin();
      int id = *it;
      setMyUnits.erase(it);
      Unit u = unit(id);
      TargetPosition target = findNextMove(u, distances);
      Pos targetPos = target.p;
      int targetDist = target.dist;
      Dir d = target.d;
      int content = board[targetPos.i][targetPos.j];
      int prevDist = distances[targetPos.i][targetPos.j];
      int prevId = ids[targetPos.i][targetPos.j];
      // If we are going to an already targeted position (but we are closer), put the unit that was going there to recalculate its movement
      if (prevDist != -1) {
        movements.erase(prevId);
        setMyUnits.insert(prevId);
      }
      int priority = 10;
      if (content == ENEMY and targetDist <= 2) {
        if (targetDist == 1) priority = 1;
        else if (targetDist == 2) priority = 20;
        else priority = 10;
        movements.insert({id, {priority, d}});
      }
      else if (content == ZOMBIE) {
        if (targetDist == 1) priority = 2; // SWITCH TO 3?
        else priority = 4;
        d = zombieBestMove(u, targetPos, targetDist, d);
        if (d != DR) movements.insert({id, {priority, d}});
      }
      else if (content == FOOD) {
        if (targetDist == 1) priority = 3;  // SWITCH TO 2? OR GET FOOD EVEN IF ZOMBIE TARGETING the dist 1?
        else priority = 10;
        Pos newPos = u.pos + d;
        Pos zombiePos;
        // If Zombie near our next move, adapt move to not get infected
        if (isZombieClose(newPos, zombiePos)) {
          int zombieDist = abs(u.pos.i-zombiePos.i) + abs(u.pos.j-zombiePos.j);
          d = zombieBestMove(u, zombiePos, zombieDist, d);
        }
        movements.insert({id, {priority, d}});
      }
      else {
        priority = 10;
        // If Zombie near our next move, adapt move to not get infected
        Pos newPos = u.pos + d;
        Pos zombiePos;
        if (isZombieClose(newPos, zombiePos)) {
          // Prio escaping from zombie
          priority = 4;
          int zombieDist = abs(u.pos.i-zombiePos.i) + abs(u.pos.j-zombiePos.j);
          d = zombieBestMove(u, zombiePos, zombieDist, d);
        }
        movements.insert({id, {priority, d}});
      }
      // Update values
      ids[targetPos.i][targetPos.j] = id;
      distances[targetPos.i][targetPos.j] = targetDist;
    }
  }

  void moveUnits() {
    map<int, Movement> movements;
    getMovements(movements);
    priority_queue<Command, vector<Command>, CompareCommand> commands;
    getCommands(movements, commands);
    executeCommands(commands);
  }

  virtual void play () {
    double st = status(me());
    if (st >= 0.9) return;
    getRoundData();
    moveUnits();
  }
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);