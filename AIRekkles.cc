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
    cWaste,               // Waste is in the cell
    cDead,                // A dead unit is in the cell
    cFood,                // Food is in the cell
    cZombie,              // A zombie is in the cell
    cEnemy,               // An enemy is in the cell
    cUnit,                // A friendly unit is in the cell
    cEmptyOwned,          // Cell is empty and owned by me
    cEmptyNotOwned,       // Cell is empty and not owned by me (owned by another player or not owned by anyone)
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

  VI units;                   // Vector that contains my units
  set<int> set_units;         // Set that contains my units (from units)
  VVC mapC = VVC(n, VC(n));   // Matrix that contains all cells content

  /**
   * Play method, invoked once per each round.
   */
  CellContent cellToCellContent(const Cell& cell) {
    if (cell.type == Waste) return cWaste;
    if (cell.food) return cFood;
    // An empty cell
    if (cell.id == -1) {
      if (cell.owner != me()) return cEmptyNotOwned;
      return cEmptyOwned; 
    }
    Unit u = unit(cell.id);
    if (u.type == Zombie) return cZombie;
    if (u.type == Dead) return cDead;
    if (u.player == me()) return cUnit;
    return cEnemy;
  }

  void getMapContent() {
    for (int i = 0; i < n; ++i) {
      for (int j = 0; j < n; ++j) {
        mapC[i][j] = cellToCellContent(cell(i, j));
      }
    }
  }
  
  void getMyUnits() {
    set_units.clear();
    units = alive_units(me());
    for (auto id : units) set_units.insert(id);
  }

  void getRoundData() {
    getMapContent();
    getMyUnits();
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

  // Returns true if the position <p> is inside map and is not Waste
  bool posOk(const Pos& p) {
    return (pos_ok(p) and mapC[p.i][p.j] != cWaste);
  }

  // Returns true if there is a dead unit in position <p>
  bool isPosDead(const Pos& p) {
    return mapC[p.i][p.j] == cDead;
  }

  struct Compare {
    bool operator() (const PositionValue& a, const PositionValue& b) {
      if (a.value != b.value) return a.value < b.value;
      return a.tp.dist >= b.tp.dist;
    }
  };

  // Returns Dir and Pos to the closest Food, Zombie or Enemy.
  // If it does not find any, returns Dir and Pos of first available cell or first empty not owned cell.
  TargetPosition findClosestUnit(const Unit& u, const VVI& distances, VVB& visited, queue<TargetPosition>& q) {
    if (units.size() <= 40) max_bfs = 50;
    else if (units.size() <= 60) max_bfs = 30;
    else max_bfs = 20;
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
      CellContent content = mapC[p.i][p.j];
      switch(content) {
        case cWaste: continue;
        case cEnemy:
          if (isInfected and dist > 2*stepsPossibleAsZombie) continue;
          if (isTargeted) {
            if (dist >= prevDist) break;
            else if (prevDist == 2 and units.size() >= 15) break;
          }
          if (true) {
            int enemyStr = strength(unit(cell(p).id).player);
            double probability = double(str)/(double(str)+double(enemyStr));
            double totalProbability = probability + 0.3;
            if (probability >= 0.75) targets.push({target, 20-dist});
            else if (probability >= 0.5) targets.push({target, 10-dist});
            else if (probability >= 0.286) targets.push({target, 3-dist});
            else continue;
          }
          break;
        case cZombie:
          if (isInfected and dist > 2.5*stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          if (dist == 1 or dist == 2) return target;
          targets.push({target, 5-dist});
          break;
        case cFood:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          targets.push({target, 25-dist});
        case cDead:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          if (dist < unit(cell({p.i, p.j}).id).rounds_for_zombie) {
            max_bfs = 2*unit(cell({p.i, p.j}).id).rounds_for_zombie;
            continue;
          }
          // If we arrive 1 step early, just at time or later: GO
          targets.push({target, 5-dist});
          break;
        case cEmptyNotOwned:
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
        if (mapC[newPos.i][newPos.j] == cEnemy and distances[newPos.i][newPos.j] == -1) return target;
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

  Dir zombieBestMove(const Pos& unitPos, const Pos& zombiePos, int dist, Dir d, int roundsForZombie) {
    if (dist == 1) return d;
    int a = zombiePos.i - unitPos.i;
    int b = zombiePos.j- unitPos.j;
    if (dist == 2 and roundsForZombie == -1) {
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

  void moveUnits() {
    // Contains first movements to perform
    map<int, Dir> firstMovements;
    // Contains last movements to perform
    map<int, Dir> lastMovements;
    // Contains movements to perform with no priority
    map<int, Dir> nextMovements;
    // Contains the id of the unit planning to go to that position
    VVI ids = VVI(n, VI(n, -1));
    // If value is -1, no unit plans to go to that position
    // Else, there is a unit planning to go to that position and the value is the steps it has to take to get there
    VVI distances = VVI(n, VI(n, -1));
    while (not set_units.empty()) {
      auto it = set_units.begin();
      int id = *it;
      set_units.erase(it);
      Unit u = unit(id);
      TargetPosition target = findNextMove(u, distances);
      Pos targetPos = target.p;
      int targetDist = target.dist;
      Dir d = target.d;
      // If we are going to an already targeted position (but we are closer), put the unit that was going there to recalculate its movement
      if (distances[targetPos.i][targetPos.j] != -1) {
        if (mapC[targetPos.i][targetPos.j] == cEnemy and distances[targetPos.i][targetPos.j] == 2) lastMovements.erase(ids[targetPos.i][targetPos.j]);
        else nextMovements.erase(ids[targetPos.i][targetPos.j]);
        set_units.insert(ids[targetPos.i][targetPos.j]);
      }
      if (mapC[targetPos.i][targetPos.j] == cEnemy and targetDist <= 2) {
        if (targetDist == 1) firstMovements.insert({id, d});
        else if (targetDist == 2) lastMovements.insert({id, d});
      }
      else if (mapC[targetPos.i][targetPos.j] == cZombie) {
        d = zombieBestMove(u.pos, targetPos, targetDist, d, u.rounds_for_zombie);
        if (d != DR) nextMovements.insert({id, d});
      }
      else nextMovements.insert({id, d});
      // Update values
      ids[targetPos.i][targetPos.j] = id;
      distances[targetPos.i][targetPos.j] = targetDist;
    }
    // First commands
    for (auto movement : firstMovements) move(movement.first, movement.second);
    // Next commands
    for (auto movement : nextMovements) move(movement.first, movement.second);
    // Last commands
    for (auto movement : lastMovements) move(movement.first, movement.second);
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