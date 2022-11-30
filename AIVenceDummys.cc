#include "Player.hh"
/**
 * Write the name of your player and save this file
 * with the same name and .cc extension.
 */
#define PLAYER_NAME VenceDummys


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
  vector<Dir> dirs = {Up, Down, Left, Right};

  VI units;             // Vector that contains my units
  set<int> set_units;   // Set that contains my units (from units)
  VVC mapC = VVC(n, VC(n));              // Matrix that contains all cells content

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

  // Returns Dir and Pos to the closest Food, Zombie or Enemy.
  // If it does not find any, returns Dir and Pos of first available cell or first empty not owned cell.
  Dir findClosestUnit(const Unit& u, const VVI& distances, VVB& visited, queue<targetPosition>& q, Pos& targetPos, int& targetDist) {


    // bool isInfected = u.rounds_for_zombie != -1;
    // int stepsPossibleAsZombie = u.rounds_for_zombie;
    int max_bfs = 20;
    int str = strength(me());
    // if (isInfected) max_bfs = 10;
    // If we don't find anything, go to the first available cell or first empty not owned cell
    bool found = false;
    targetPos = q.front().p;
    Dir dirToEmptyCell = q.front().d;
    targetDist = q.front().dist;
    while (not q.empty() and (max_bfs != q.front().dist or not found)) {
      Pos p = q.front().p;
      Dir d = q.front().d;
      int dist = q.front().dist;
      q.pop();

      if (visited[p.i][p.j]) continue;
      visited[p.i][p.j] = true;
      if (mapC[p.i][p.j] == cWaste) continue;
      // If it is already targeted, only go if your distance is less
      if (distances[p.i][p.j] != -1) {
        if (dist < distances[p.i][p.j]) {
          targetPos = p;
          targetDist = dist;
          return d;
        }
        continue;
      }
      if (mapC[p.i][p.j] == cFood or mapC[p.i][p.j] == cZombie) {
        targetPos = p;
        targetDist = dist;
        return d;
      }
      if (mapC[p.i][p.j] == cEnemy) {
        int enemyStrength = strength(unit(cell(p).id).player);
        // If more than 28,6% chance of winning the fight, GO. If I attack first, would have 0.3+0.7*chance of winning. Rn is at 50%.
        double chance = 0.286;
        if ((1-chance)*str >= chance*enemyStrength) {
          targetPos = p;
          targetDist = dist;
          return d;
        }
        // Should I reconsider to always attacking? If I go to a position close to the enemy, I would be killed 100% bc he attacks first and has more chances of winning the fight
        continue;
      }
      // At the moment, we do not follow our infected units
      // if (mapC[p.i][p.j] == cUnit) {
      //   // If it's infected, go
      //   if (unit(cell({p.i, p.j}).id).rounds_for_zombie != -1 and dist >= unit(cell({p.i, p.j}).id).rounds_for_zombie) {
      //     targetPos = p;
      //     targetDist = dist;
      //     return d; 
      //   }
      //   continue;
      // }

      if (mapC[p.i][p.j] == cDead) {
        // If we arrive 1 step early, just at time, or later, GO
        if (dist >= unit(cell({p.i, p.j}).id).rounds_for_zombie) {
          targetPos = p;
          targetDist = dist;
          return d;
        }
        continue;
      }
      // if (isInfected) {
      //   if (map[p.i][p.j] == cDead) {
      //     if (dist >= unit(cell({p.i, p.j}).id).rounds_for_zombie and dist <= stepsPossibleAsZombie) {
      //       targetPos = p;
      //       return d;
      //     }
      //     else continue;
      //   }
      //   else if (map[p.i][p.j] == cFood) {
      //     if (dist <= stepsPossibleAsZombie) {
      //       targetPos = p;
      //       return d;
      //     }
      //   }
      //   else if (map[p.i][p.j] == cZombie or map[p.i][p.j] == cEnemy) {
      //     targetPos = p;
      //     return d;
      //   }
      // }
      // else {
      //   // Dead unit
      //   if (map[p.i][p.j] == cDead) {
      //     // If we arrive 1 step early, just at time, or later, GO
      //     if (dist >= unit(cell({p.i, p.j}).id).rounds_for_zombie) {
      //       targetPos = p;
      //       return d;
      //     }
      //     // If we arrive 2 or more steps early, go to another direction
      //     else continue;
      //   }
      //   // If we found what we want, go there
      //   if (map[p.i][p.j] == cFood or map[p.i][p.j] == cZombie) {
      //     targetPos = p;
      //     return d;
      //   }
      // }
      // Find first empty not owned cell
      if (not found and mapC[p.i][p.j] == cEmptyNotOwned) {
        dirToEmptyCell = d;
        targetPos = p;
        targetDist = dist;
        found = true;
      }
      // Check all four directions in a random order: Up, Down, Left and Right
      random_shuffle(dirs.begin(), dirs.end());
      for (auto dir : dirs) {
        Pos newPos = p + dir;
        if (pos_ok(newPos) and not visited[newPos.i][newPos.j]) q.push({newPos, d, dist + 1});
      }
    }
    return dirToEmptyCell;
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
      if (pos_ok(newPos) and mapC[newPos.i][newPos.j] != cWaste) {
        // If there's an enemy next to us, prioritize movement to kill him.
        if (mapC[newPos.i][newPos.j] == cEnemy) {
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

  struct Movement {
    int id;
    Dir d;
    int dist;
  };

  struct Compare {
    bool operator() (const Movement& a, const Movement& b) const {
      if (a.dist != b.dist) return a.dist > b.dist;
      return a.id < b.id;
    }
  };

  void moveUnits() {
    // Contains first movements to perform
    vector<Movement> priorityMovements;
    // Contains last movements to perform
    vector<Movement> lastMovements;
    // Contains movements to performs with no priority
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
      if (targetDist == 1 and (mapC[targetPos.i][targetPos.j] == cEnemy or mapC[targetPos.i][targetPos.j] == cFood)) {
        Movement m;
        m.d = d;
        m.id = id;
        m.dist = targetDist;
        priorityMovements.push_back(m);
      }
      else if (targetDist == 2 and mapC[targetPos.i][targetPos.j] == cEnemy) {
        Movement m;
        m.d = d;
        m.id = id;
        m.dist = targetDist;
        lastMovements.push_back(m);
      }
      // If we are going to an already targeted position, recalculate the movement to the unit that was going there
      else if (distances[targetPos.i][targetPos.j] != -1) {
        nextMovements.erase(ids[targetPos.i][targetPos.j]);
        set_units.insert(ids[targetPos.i][targetPos.j]);
        nextMovements.insert({id, d});
      }
      else nextMovements.insert({id, d});
      // Update values
      ids[targetPos.i][targetPos.j] = id;
      distances[targetPos.i][targetPos.j] = targetDist;
    }
    // First commands where dist == 1 and target is food or enemy
    for (auto movement : priorityMovements) move(movement.id, movement.d);
    // Next commands
    for (auto movement : nextMovements) {
      int id = movement.first;
      Dir d = movement.second;
      move(id, d);
    }
    // Last commands where dist == 2 and target is enemy. Used to counterattack
    for (auto movement : lastMovements) move(movement.id, movement.d);
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
