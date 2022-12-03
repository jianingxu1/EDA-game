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

  struct targetPosition {
    Pos p;
    Dir d;
    int dist;
  };

  // Returns true if the position <p> is inside map and is not Waste
  bool posOk(const Pos& p) {
    return (pos_ok(p) and mapC[p.i][p.j] != cWaste);
  }

  // Returns true if there is a dead unit in position <p>
  bool isPosDead(const Pos& p) {
    return mapC[p.i][p.j] == cDead;
  }

  // Returns Dir and Pos to the closest Food, Zombie or Enemy.
  // If it does not find any, returns Dir and Pos of first available cell or first empty not owned cell.
  Dir findClosestUnit(const Unit& u, const VVI& distances, VVB& visited, queue<targetPosition>& q, Pos& targetPos, int& targetDist) {
    targetPosition firstEmpty = q.front();
    bool found = false;
    max_bfs = 50;
    bool isInfected = u.rounds_for_zombie != -1;
    int stepsPossibleAsZombie = u.rounds_for_zombie;
    if (isInfected) max_bfs = stepsPossibleAsZombie*2.5 + 1;
    // If we don't find anything, go to the first available cell or first empty not owned cell
    while (not q.empty()) {
      Pos p = q.front().p;
      Dir d = q.front().d;
      int dist = q.front().dist;
      q.pop();
      CellContent content = mapC[p.i][p.j];
      if (visited[p.i][p.j]) continue;
      visited[p.i][p.j] = true;
      targetPos = p;
      targetDist = dist;
      bool isTargeted = distances[p.i][p.j] != -1;
      int prevDist = distances[p.i][p.j];
      // If continue, treat it as a wall and stop searching there
      // If break, do not go there but continue searching
      // Else, go there
      switch(content) {
        case cWaste: continue;
        case cEnemy:
          if (isInfected and dist > 2*stepsPossibleAsZombie) continue;
          if (isTargeted) {
            if (dist >= prevDist) break;
            // If the unit going there is doing a counterattack and we have a lot of units, do not replace
            else if (prevDist == 2 and units.size() >= 15) break;
          }
          return d;
        case cZombie:
          if (isInfected and dist > 2.5*stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          return d;
        case cFood:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          return d;
        case cDead:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          if (dist < unit(cell({p.i, p.j}).id).rounds_for_zombie) {
            max_bfs = 2*unit(cell({p.i, p.j}).id).rounds_for_zombie;
            continue;
          }
          // If we arrive 1 step early, just at time or later: GO
          return d;
        case cEmptyNotOwned:
          if (isInfected and dist > stepsPossibleAsZombie) continue;
          if (isTargeted and dist >= prevDist) break;
          // Find first not owned empty cell
          if (not found) {
            firstEmpty.d = d;
            firstEmpty.p = p;
            firstEmpty.dist = dist;
            found = true;
          }
          break;
        default: break;
      }
      // Check all four directions in a random order
      random_shuffle(dirs.begin(), dirs.end());
      for (auto dir : dirs) {
        Pos newPos = p + dir;
        if (pos_ok(newPos) and not visited[newPos.i][newPos.j]) q.push({newPos, d, dist + 1});
      }
    }
    targetPos = firstEmpty.p;
    targetDist = firstEmpty.dist;
    return firstEmpty.d;
  }

  // Mark position as Targeted and return the direction to go there
  Dir findNextMove(const Unit& u, const VVI& distances, Pos& targetPos, int& targetDist) {
    VVB visited(n, VB(n, false));
    queue<targetPosition> q;
    visited[u.pos.i][u.pos.j] = true;

    // Initial movements
    random_shuffle(dirs.begin(), dirs.end());
    for (auto d : dirs) {
      Pos newPos = u.pos + d;
      if (posOk(newPos)) {
        // If there's an enemy next to us, prioritize movement to kill him.
        if (mapC[newPos.i][newPos.j] == cEnemy and distances[newPos.i][newPos.j] == -1) {
          targetPos = newPos;
          targetDist = 1;
          return d;
        }
        q.push({newPos, d, 1});
      }
    }

    // If only one direction is possible, go there
    if (q.size() == 1) {
      targetPos = q.front().p;
      targetDist = q.front().dist;
      return q.front().d;
    }
    return findClosestUnit(u, distances, visited, q, targetPos, targetDist);
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

  string CCToString(CellContent content) {
    if (content == cDead) return "Dead";
    else if (content == cEnemy) return "Enemy";
    else if (content == cFood) return "Food";
    else if (content == cEmptyNotOwned) return "EmptyNotOwned";
    else if (content == cZombie) return "Zombie";
    else if (content == cUnit) return "Ally";
    return "EmptyOwned";
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
      Pos targetPos;
      int targetDist;
      Dir d = findNextMove(u, distances, targetPos, targetDist);

      // Debugging
      // if (round() >= 123 and round() <= 130) {
      //   if (u.pos.i > 15 and u.pos.i < 30 and u.pos.j > 50) {
      //     cerr << "ID: " << id << "\tCurrent Position: (" << u.pos.i << ", " << u.pos.j << ")" << endl;
      //     cerr << "\tTarget Pos: (" << targetPos.i << ", " << targetPos.j << ")\tDistance: " << targetDist << "\tPrevDist: " << distances[targetPos.i][targetPos.j] << "\tContent: " << CCToString(mapC[targetPos.i][targetPos.j]) << endl;
      //   }
      // }

      // If we are going to an already targeted position (but we are closer), put the unit that was going there to recalculate its movement
      if (distances[targetPos.i][targetPos.j] != -1) {
        if (mapC[targetPos.i][targetPos.j] == cEnemy and distances[targetPos.i][targetPos.j] == 2) lastMovements.erase(ids[targetPos.i][targetPos.j]);
        else nextMovements.erase(ids[targetPos.i][targetPos.j]);
        set_units.insert(ids[targetPos.i][targetPos.j]);
        // cerr << "targeted" << endl;
      }

      if (mapC[targetPos.i][targetPos.j] == cEnemy and targetDist <= 2) {
        if (targetDist == 1) firstMovements.insert({id, d});
        else if (targetDist == 2) lastMovements.insert({id, d});
        // If dist == 3, do not move
        // cerr << "enemy opti" << endl;
      }
      else if (mapC[targetPos.i][targetPos.j] == cZombie) {
        d = zombieBestMove(u.pos, targetPos, targetDist, d, u.rounds_for_zombie);
        if (d != DR) nextMovements.insert({id, d});
        // cerr << "zombie opti" << endl;
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
    getRoundData();
    moveUnits();
  }
};

/**
 * Do not modify the following line.
 */
RegisterPlayer(PLAYER_NAME);