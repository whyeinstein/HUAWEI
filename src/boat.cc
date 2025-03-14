#include "boat.h"

#include <algorithm>
#include <iostream>

#include "berth.h"
#include "decision.h"
#include "map_controller.h"
#include "param.h"
#include "rent_controller.h"
int Boat::boat_capacity = 0;
std::map<std::pair<Location, int>, std::vector<int>> Boat::d_path;
#ifdef DEBUG
char Boat::dir_str[4][10] = {"右", "左", "上", "下"};
#endif
extern int id;

Boat::Boat() { num = 0; }
Boat::Boat(int &id, int &goods_num, int &x, int &y, int &direction, int &status)
    : id_(id),
      num(goods_num),
      x(x),
      y(y),
      direction(direction),
      status(status) {
  area_id = MapController::GetInstance()->FindArea(x * n + y, false);
}

// 顺时针旋转
void Boat::DoClockwiseRotate() {
  Decision decision(DECISION_TYPE_BOAT_ROT, id_, DECISION_BOAT_ROT_CLOCKWISE);
  DecisionManager::GetInstance()->q_decision.push(decision);
  // RemoveFirst();
#ifdef DEBUG
  std::cerr << id_ << " 船顺时针旋转" << std::endl;
#endif
}

// 逆时针旋转
void Boat::DoCounterclockwiseRotate() {
  Decision decision(DECISION_TYPE_BOAT_ROT, id_,
                    DECISION_BOAT_ROT_COUNTERCLOCKWISE);
  DecisionManager::GetInstance()->q_decision.push(decision);
  // RemoveFirst();
#ifdef DEBUG
  std::cerr << id_ << " 船逆时针旋转" << std::endl;
#endif
}

// 往前走
void Boat::DoShip() {
  Decision decision(DECISION_TYPE_BOAT_SHIP, id_);
  DecisionManager::GetInstance()->q_decision.push(decision);
  // RemoveFirst();
#ifdef DEBUG
  std::cerr << id_ << " 船往前走(向" << dir_str[direction] << ")" << std::endl;
#endif
}

// 靠泊
void Boat::DoBerth() {
  Decision decision(DECISION_TYPE_BOAT_BERTH, id_);
  DecisionManager::GetInstance()->q_decision.push(decision);
  path.clear();
#ifdef DEBUG
  std::cerr << id_ << " 船靠泊：" << pos << std::endl;
#endif
}

// 重置到主航道
void Boat::DoDept() {
  Decision decision(DECISION_TYPE_BOAT_DEPT, id_);
  DecisionManager::GetInstance()->q_decision.push(decision);
  path.clear();
#ifdef DEBUG
  std::cerr << id_ << " 船重置到主航道" << std::endl;
#endif
}

CollisionBox::CollisionBox(int core_x, int core_y, int direction) {
  switch (direction) {
    case BOAT_DIRECTION_RIGHT:
      l_x = core_x;
      l_y = core_y;
      r_x = core_x + 1;
      r_y = core_y + 2;
      break;
    case BOAT_DIRECTION_LEFT:
      l_x = core_x - 1;
      l_y = core_y - 2;
      r_x = core_x;
      r_y = core_y;
      break;
    case BOAT_DIRECTION_UP:
      l_x = core_x - 2;
      l_y = core_y;
      r_x = core_x;
      r_y = core_y + 1;
      break;
    case BOAT_DIRECTION_DOWN:
      l_x = core_x;
      l_y = core_y - 1;
      r_x = core_x + 2;
      r_y = core_y;
      break;
    default:
#ifdef DEBUG
      std::cerr << "创建CollisionBox对象:错误的direction" << direction
                << std::endl;
#endif
      break;
  }
}

CollisionBox::CollisionBox(int core_x, int core_y, int direction,
                           int next_direction) {
  Location loc(core_x, core_y, direction);
  switch (MoveType(direction, next_direction)) {
    case DECISION_BOAT_SHIP:
      loc = loc.Ship();
      break;
    case DECISION_BOAT_ROT_CLOCKWISE:
      loc = loc.Clockwise();
      break;
    case DECISION_BOAT_ROT_COUNTERCLOCKWISE:
      loc = loc.CounterClockwise();
      break;
  }
  *this = CollisionBox(loc.x, loc.y, loc.boat_direction);
}

// 是否撞边界
bool CollisionBox::IsCollision() {
  if (l_x < 1 || l_y < 1 || r_x > n || r_y > n) {
    // 出界了
    return true;
  }
  for (int i = l_x; i <= r_x; ++i) {
    for (int j = l_y; j <= r_y; ++j) {
      if (!MapController::GetInstance()->CanBoatReach(i, j)) {
        return true;
      }
    }
  }
  return false;
}

// 两个对象是否相撞
bool CollisionBox::JudgeCollision(const CollisionBox &first,
                                  const CollisionBox &second) {
  if (first.l_x > second.r_x || first.r_x < second.l_x ||
      first.l_y > second.r_y || first.r_y < second.l_y) {
    return false;
  }
  return true;
}

// 删除path的第一个点
void Boat::RemoveFirst() {
  if (!path.empty()) {
    path.erase(path.begin());
  }
}

// 判断能否交货
bool Boat::DeliveryCond() {
  if (pos == -2) {
    // 才购买的船
    return false;
  }
  if (pos == -1) {
    if (num) {
      // 交货途中碰撞处理过

#ifdef DEBUG
      std::cerr << "交货途中碰撞处理过，重新交货" << std::endl;
#endif
      return true;
    } else {
      return false;
    }
  }
  std::vector<Berth> &berth = MapController::GetInstance()->berth;
  if (id > 15000 - berth[pos].transport_time -
               DynamicParam::GetInstance()->GetTolerantLeaveTime()) {
#ifdef DEBUG
    std::cerr << berth[pos].boat_id
              << " 船离开了，剩余货物数量: " << berth[pos].goods_num
              << " 船货物数量: " << num << std::endl;
#endif
    return true;
  }
  if (berth[pos].goods_num > 0) {
    // 能装满就装满了走
    return num >= boat_capacity;
  }
  return false;
  // 微调快满的船又去换泊位
  return num >=
         boat_capacity - DynamicParam::GetInstance()->GetBoatCapacityReduce();
}

// 寻找交货点
void Boat::FindDeliveryPoint() {
  // 去交货
  auto &berth = MapController::GetInstance()->berth;
  if (pos != -1 && berth[pos].path.find(-1) != berth[pos].path.end()) {
    // 路径存过
#ifdef DEBUG
    std::cerr << "路径复用：回交货点" << std::endl;
#endif
    path = berth[pos].path[-1];
  } else {
    auto &delivery_point = MapController::GetInstance()->delivery_point;
    int delivery_id = MapController::GetInstance()->nearest_delivery[x][y];

#ifdef DEBUG
    std::cerr << "------- start astar -------" << std::endl;
    std::cerr << "(" << x << "," << y << ")---->("
              << delivery_point[delivery_id].x << ","
              << delivery_point[delivery_id].y << ")"
              << "方向:" << dir_str[direction] << std::endl;
#endif
    Astar astar(x, y, delivery_point[delivery_id].x,
                delivery_point[delivery_id].y, direction);
    astar.AstarSearch(path);
    if (pos != -1) {
      berth[pos].path[-1] = path;
      berth[pos].transport_time = path.size();
    }
  }

  // 重置老泊位
  if (pos != -1) {
    berth[pos].boat_id = -1;
  }
  pos = -1;
#ifdef DEBUG
  std::cerr << id_ << " 船去交货, path size: " << path.size()
            << " 货物数量: " << num << " 金额：" << money << std::endl;
#endif
  money = 0;
}

/*
 * 船找泊位
 * todo: 在非泊位点不准确的can_load也许会出问题
 */
void Boat::FindBerth() {
  auto &berth = MapController::GetInstance()->berth;
  int size = berth.size();
  int berth_id = -1;

  Location loc(x, y);
  int now_berth_id = -1;
  auto &location_to_berth_id =
      MapController::GetInstance()->location_to_berth_id;
  if (location_to_berth_id.find(loc) != location_to_berth_id.end()) {
    now_berth_id = location_to_berth_id[loc];
  }
  auto &nearest_delivery = MapController::GetInstance()->nearest_delivery;
  double max_per_money = 0;

  for (int i = 0; i < size; ++i) {
    if (berth[i].area_id != area_id || berth[i].boat_id != -1 ||
        !berth[i].goods_num) {
      continue;
    }
    int temp_transport_time = berth[i].transport_time;  // 回家的时间
    int distance = 0;
    if (pos == -1) {
      // 在交货点，算来回时间
      if (nearest_delivery[x][y] == nearest_delivery[berth[i].x][berth[i].y]) {
        // 泊位记录的transport_time是该交货点
        distance = temp_transport_time;
      } else {
        // 泊位记录的transport_time不是该交货点
        distance = std::abs(x - berth[i].GetNearestX(x)) +
                   std::abs(y - berth[i].GetNearestY(y));
      }
    } else if (now_berth_id == -1) {
      // 当前没有在交货点和泊位中
      distance = std::abs(x - berth[i].GetNearestX(x)) +
                 std::abs(y - berth[i].GetNearestY(y));
    } else if (berth[now_berth_id].path.find(i) !=
               berth[now_berth_id].path.end()) {
      // 当前泊位存了去目标泊位的路径
      distance = berth[now_berth_id].path[i].size();
    } else if (berth[i].path.find(now_berth_id) != berth[i].path.end()) {
      // 目标泊位存了来所在泊位的路径
      distance = berth[i].path[now_berth_id].size();
    } else {
      // 在泊位中但还没算过路径
      distance = std::abs(berth[i].GetNearestX(x) - x) +
                 std::abs(berth[i].GetNearestY(y) - y);
    }
    temp_transport_time += distance;
    int time = 15000 - id - temp_transport_time -
               DynamicParam::GetInstance()
                   ->GetTolerantLeaveTime();  // 过去剩余的装载时间
    int capacity = std::min(time * berth[i].loading_speed, boat_capacity - num);
    int next_money = berth[i].GetIdealMoney(capacity);
    double next_per_money = 1.0 * next_money / distance;
    if (next_per_money > max_per_money) {
      max_per_money = next_per_money;
      berth_id = i;
    }
  }

  //   // 先看有没有泊位能让自己填满
  //   int transport_time = 1010;
  //   // 移动到泊位再回家的预估时间
  //   for (int i = 0; i < size; ++i) {
  //     if (berth[i].area_id != area_id || berth[i].boat_id != -1 ||
  //         !berth[i].goods_num) {
  //       continue;
  //     }
  //     int temp_transport_time = berth[i].transport_time;
  //     int distance = 0;
  //     if (pos == -1) {
  //       // 在交货点，算来回时间
  //       if (nearest_delivery[x][y] ==
  //       nearest_delivery[berth[i].x][berth[i].y]) {
  //         // 泊位记录的transport_time是该交货点
  //         distance = temp_transport_time;
  //       } else {
  //         // 泊位记录的transport_time不是该交货点
  //         distance = std::abs(x - berth[i].GetNearestX(x)) +
  //                    std::abs(y - berth[i].GetNearestY(y));
  //       }
  //     } else if (now_berth_id == -1) {
  //       // 当前没有在交货点和泊位中
  //       distance = std::abs(x - berth[i].GetNearestX(x)) +
  //                  std::abs(y - berth[i].GetNearestY(y));
  //     } else if (berth[now_berth_id].path.find(i) !=
  //                berth[now_berth_id].path.end()) {
  //       // 当前泊位存了去目标泊位的路径
  //       distance = berth[now_berth_id].path[i].size();
  //     } else if (berth[i].path.find(now_berth_id) != berth[i].path.end()) {
  //       // 目标泊位存了来所在泊位的路径
  //       distance = berth[i].path[now_berth_id].size();
  //     } else {
  //       // 在泊位中但还没算过路径
  //       distance = std::abs(berth[i].GetNearestX(x) - x) +
  //                  std::abs(berth[i].GetNearestY(y) - y);
  //     }
  //     temp_transport_time += distance;

  //     int time = 15000 - id - temp_transport_time -
  //                DynamicParam::GetInstance()->GetTolerantLeaveTime();

  //     int can_load = std::min(time * berth[i].loading_speed,
  //     berth[i].goods_num); if (can_load >= boat_capacity - num) {
  //       // 泊位i可以把该船装满
  //       if (berth_id == -1 || temp_transport_time < transport_time) {
  //         // 选离家近的
  //         berth_id = i;
  //         transport_time = temp_transport_time;
  //       }
  //     }
  //   }

  //   if (berth_id == -1) {
  //     // 没有能装满该船的泊位
  //     int max_goods_num = 0;
  //     for (int i = 0; i < size; ++i) {
  //       if (berth[i].area_id != area_id || berth[i].boat_id != -1 ||
  //           !berth[i].goods_num) {
  //         continue;
  //       }

  //       int temp_transport_time = berth[i].transport_time;
  //       int distance = 0;
  //       if (pos == -1) {
  //         // 在交货点，算来回时间
  //         if (nearest_delivery[x][y] ==
  //             nearest_delivery[berth[i].x][berth[i].y]) {
  //           // 泊位记录的transport_time是该交货点
  //           distance = temp_transport_time;
  //         } else {
  //           // 泊位记录的transport_time不是该交货点
  //           distance = std::abs(x - berth[i].GetNearestX(x)) +
  //                      std::abs(y - berth[i].GetNearestY(y));
  //         }
  //       } else if (now_berth_id == -1) {
  //         // 当前没有在交货点和泊位中
  //         distance = std::abs(x - berth[i].GetNearestX(x)) +
  //                    std::abs(y - berth[i].GetNearestY(y));
  //       } else if (berth[now_berth_id].path.find(i) !=
  //                  berth[now_berth_id].path.end()) {
  //         // 当前泊位存了去目标泊位的路径
  //         distance = berth[now_berth_id].path[i].size();
  //       } else if (berth[i].path.find(now_berth_id) != berth[i].path.end()) {
  //         // 目标泊位存了来所在泊位的路径
  //         distance = berth[i].path[now_berth_id].size();
  //       } else {
  //         // 在泊位中但还没算过路径
  //         distance = std::abs(berth[i].GetNearestX(x) - x) +
  //                    std::abs(berth[i].GetNearestY(y) - y);
  //       }
  //       temp_transport_time += distance;
  //       int time = 15000 - id - temp_transport_time -
  //                  DynamicParam::GetInstance()->GetTolerantLeaveTime();

  //       int can_load =
  //           std::min(time * berth[i].loading_speed, boat_capacity - num);
  //       can_load = std::min(can_load, berth[i].goods_num);
  //       // 选货物最多的
  //       if (can_load > max_goods_num) {
  //         max_goods_num = can_load;
  //         berth_id = i;
  //       }
  //     }
  //   } else {
  // #ifdef DEBUG
  //     std::cerr << "选择了回家近的泊位" << std::endl;
  // #endif
  //   }

  if (pos == -1 && berth_id == -1) {
    // 最后阶段 随便找个泊位待着
    int temp_transport_time = 500;
    for (int i = 0; i < size; ++i) {
      if (berth[i].area_id != area_id || berth[i].boat_id != -1) {
        continue;
      }
      if (nearest_delivery[x][y] == nearest_delivery[berth[i].x][berth[i].y] &&
          berth[i].transport_time < temp_transport_time) {
        berth_id = i;
        temp_transport_time = berth[i].transport_time;
      }
    }
#ifdef DEBUG
    std::cerr << id_ << " 最后阶段 找个最近的泊位待着：" << berth_id
              << std::endl;
#endif
  }

  if (berth_id > -1) {
    // 判断是否处于泊位中

    if (status == BOAT_STATUS_LOADING && now_berth_id != -1) {
      // 目前处于某泊位中
      if (berth[now_berth_id].path.find(berth_id) !=
          berth[now_berth_id].path.end()) {
        // 路径已经算过了

#ifdef DEBUG
        std::cerr << "路径复用：泊位" << std::endl;
#endif
        path = berth[now_berth_id].path[berth_id];
      } else {
        // 路径没算过
#ifdef DEBUG
        std::cerr << "------- start astar -------" << std::endl;
        std::cerr << "(" << x << "," << y << ")---->(" << berth[berth_id].x
                  << "," << berth[berth_id].y << ")"
                  << "方向:" << Boat::dir_str[direction] << std::endl;
#endif
        Astar astar(x, y, berth[berth_id].x, berth[berth_id].y, direction);
        astar.AstarSearch(path);
        berth[now_berth_id].path[berth_id] = path;
      }

      double now_per_money =
          1.0 * money /
          (total_transport_time + berth[now_berth_id].transport_time);
      int time = 15000 - path.size() - berth[berth_id].transport_time -
                 DynamicParam::GetInstance()->GetTolerantLeaveTime();
      int capacity =
          std::min(boat_capacity - num, time * berth[berth_id].loading_speed);
      int next_money = money + berth[berth_id].GetIdealMoney(capacity);
      double next_per_money =
          1.0 * next_money /
          (total_transport_time + path.size() + berth[berth_id].transport_time);
      if (next_per_money < now_per_money) {
        FindDeliveryPoint();
#ifdef DEBUG
        std::cerr << "现在回家性价比更高:" << num << "目标泊位货物："
                  << berth[berth_id].goods_num << std::endl;
#endif
        return;
      } else {
#ifdef DEBUG
        std::cerr << "继续换泊位 " << num << std::endl;
#endif
      }
    } else {
      // 目前不处于泊位中

      // 交货点的下标
      int d_index = -1;
      auto &delivery_point = MapController::GetInstance()->delivery_point;
      int delivery_point_size = delivery_point.size();
      Location loc(x, y);
      for (int i = 0; i < delivery_point_size; ++i) {
        if (delivery_point[i] == loc) {
          d_index = i;
          break;
        }
      }

      if (d_index != -1) {
        // 目前在交货点处
        std::pair<Location, int> task(
            std::make_pair(Location(x, y, direction), berth_id));
        if (d_path.find(task) != d_path.end()) {
          // 路径存过

#ifdef DEBUG
          std::cerr << "路径复用：交货点(" << loc.x << "," << loc.y
                    << ") 起始方向:" << dir_str[direction] << std::endl;
#endif
          path = d_path[task];
        } else {
          // 路径没存过

#ifdef DEBUG
          std::cerr << "------- start astar -------" << std::endl;
          std::cerr << "(" << x << "," << y << ")---->(" << berth[berth_id].x
                    << "," << berth[berth_id].y << ")"
                    << "方向:" << Boat::dir_str[direction] << std::endl;
#endif
          Astar astar(x, y, berth[berth_id].x, berth[berth_id].y, direction);
          astar.AstarSearch(path);
          d_path[task] = path;
        }
      } else {
#ifdef DEBUG
        std::cerr << "------- start astar -------" << std::endl;
        std::cerr << "(" << x << "," << y << ")---->(" << berth[berth_id].x
                  << "," << berth[berth_id].y << ")"
                  << "方向:" << Boat::dir_str[direction] << std::endl;
#endif
        Astar astar(x, y, berth[berth_id].x, berth[berth_id].y, direction);
        astar.AstarSearch(path);
      }
    }

    if (pos > -1) {
      // 重置老泊位
      berth[pos].boat_id = -1;
    }
    berth[berth_id].boat_id = id_;
    pos = berth_id;
#ifdef DEBUG
    std::cerr << id_ << " 船去泊位 " << pos << ", path size:" << path.size()
              << " 货物数量: " << num << std::endl;
#endif
  }
}

// 判断是否处于主航道
bool CollisionBox::IsLocatedOnMainRoute() {
  auto &map_instance = MapController::GetInstance();
  for (int i = l_x; i <= r_x; ++i) {
    for (int j = l_y; j <= r_y; ++j) {
      if (map_instance->IsMainChannel(i, j)) {
        return true;
      }
    }
  }
  return false;
}

// 判断是否完全处于主航道
bool CollisionBox::IsCompletelyLocatedOnMainRoute() {
  auto &map_instance = MapController::GetInstance();
  for (int i = l_x; i <= r_x; ++i) {
    for (int j = l_y; j <= r_y; ++j) {
      if (!map_instance->IsMainChannel(i, j)) {
        return false;
      }
    }
  }
  return true;
}

// 给该船下移动指令
void Boat::DoMove() {
  // 继续走
  switch (MoveType(direction, path[0])) {
    case DECISION_BOAT_SHIP:
      DoShip();
      break;
    case DECISION_BOAT_ROT_CLOCKWISE:
      DoClockwiseRotate();
      break;
    case DECISION_BOAT_ROT_COUNTERCLOCKWISE:
      DoCounterclockwiseRotate();
      break;
  }
}

/*
 * 行动类型
 * @param origin_direction 初始方向
 * @param next_direction 下一步的方向
 * @return 行动类型
 */
int MoveType(int origin_direction, int next_direction) {
  if (origin_direction == next_direction) {
    return DECISION_BOAT_SHIP;
  }
  switch (origin_direction) {
    case BOAT_DIRECTION_RIGHT:
      if (next_direction == BOAT_DIRECTION_DOWN) {
        return DECISION_BOAT_ROT_CLOCKWISE;
      } else if (next_direction == BOAT_DIRECTION_UP) {
        return DECISION_BOAT_ROT_COUNTERCLOCKWISE;
      }
      break;
    case BOAT_DIRECTION_LEFT:
      if (next_direction == BOAT_DIRECTION_UP) {
        return DECISION_BOAT_ROT_CLOCKWISE;
      } else if (next_direction == BOAT_DIRECTION_DOWN) {
        return DECISION_BOAT_ROT_COUNTERCLOCKWISE;
      }
      break;
    case BOAT_DIRECTION_UP:
      if (next_direction == BOAT_DIRECTION_RIGHT) {
        return DECISION_BOAT_ROT_CLOCKWISE;
      } else if (next_direction == BOAT_DIRECTION_LEFT) {
        return DECISION_BOAT_ROT_COUNTERCLOCKWISE;
      }
      break;
    case BOAT_DIRECTION_DOWN:
      if (next_direction == BOAT_DIRECTION_LEFT) {
        return DECISION_BOAT_ROT_CLOCKWISE;
      } else if (next_direction == BOAT_DIRECTION_RIGHT) {
        return DECISION_BOAT_ROT_COUNTERCLOCKWISE;
      }
      break;
  }
#ifdef DEBUG
  std::cerr << "IsClockwise方向错误" << std::endl;
#endif
  return DECISION_BOAT_ROT_CLOCKWISE;
}

/*
 * 解决碰撞
 * @param q 解决步骤
 * @param new_path 新路径
 * @return 是否成功解决
 */
bool Boat::SolveCollision(std::queue<int> q, std::vector<int> &path) {
  path.clear();
  Location loc(x, y, direction);  // 初始位置
  while (!q.empty()) {
    int temp = q.front();
    q.pop();
    switch (temp) {
      case DECISION_BOAT_ROT_CLOCKWISE:
        loc = loc.Clockwise();
        break;
      case DECISION_BOAT_ROT_COUNTERCLOCKWISE:
        loc = loc.CounterClockwise();
        break;
      case DECISION_BOAT_SHIP:
        loc = loc.Ship();
        break;
    }
    CollisionBox collisionbox(loc.x, loc.y, loc.boat_direction);
    if (collisionbox.IsCollision()) {
      return false;
    }
    path.push_back(loc.boat_direction);
  }
  return true;
}

// 是否包含某点
bool CollisionBox::IsInclude(int x, int y) {
  return !(x < l_x || x > r_x || y < l_y || y > r_y);
}