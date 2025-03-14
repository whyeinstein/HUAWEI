#include "input_controller.h"

#include <chrono>
#include <cstdio>
#include <iostream>

#include "berth.h"
#include "boat.h"
#include "goods.h"
#include "map_controller.h"
#include "param.h"
#include "rent_controller.h"
#ifdef DEBUG
extern FILE* debug_command_file;
extern FILE* debug_map_file;
#endif

InputController* InputController::instance_ = nullptr;

InputController*& InputController::GetInstance() {
  if (!instance_) {
    instance_ = new InputController();
  }
  return instance_;
}
int money;  // 得分
int id;     // 帧号

// 初始化
void InputController::Init() {
  char(&ch)[N][N] = MapController::GetInstance()->ch;
  // 地图数据
  for (int i = 1; i <= n; i++) scanf("%s", ch[i] + 1);

#ifdef DEBUG
  for (int i = 1; i <= n; i++) {
    for (int j = 1; j <= n; j++) {
      fprintf(debug_map_file, "%c", ch[i][j]);
    }
    fprintf(debug_map_file, "\n");
  }
#endif

  MapController::GetInstance()->InitMapData();

#ifdef DEBUG
  fprintf(debug_map_file, "初始化地图数据完毕！\n");
#endif

  // 泊位数据
  std::vector<Berth>& berth = MapController::GetInstance()->berth;
  std::queue<std::pair<Location, int>> q;
  int berth_num;
  scanf("%d", &berth_num);
#ifdef FILLBERTH
  int count = 0;
  int maxindex = 0;
#endif
  for (int i = 0; i < berth_num; ++i) {
    int id, x, y, loading_speed;
    scanf("%d%d%d%d", &id, &x, &y, &loading_speed);
#ifdef FILLBERTH
    if (i == 3 || i == 4) {
      // 填充海域
      maxindex = i;
      count++;
      MapController::GetInstance()->FillMap(x + 1, y + 1);
      continue;
    } else if (i > maxindex)
      id -= count;
#endif
    berth.push_back(Berth(id, ++x, ++y, loading_speed));
    MapController::GetInstance()->InitBerthMap(id, x, y);
    // 处理nearest_berth
    MapController::GetInstance()->nearest_berth[x][y] = id;
    q.push(std::make_pair(Location(x, y), id));

#ifdef DEBUG
    fprintf(debug_map_file,
            "泊位 %d: x = %d, y = %d, loading_speed = "
            "%d\n",
            id, berth[id].x, berth[id].y, berth[id].loading_speed);
#endif
  }

#ifdef DEBUG
  fprintf(debug_map_file, "泊位数据处理完毕！\n");
  // auto start = std::chrono::high_resolution_clock::now();
#endif

  // 初始化离每个点最近的泊位id
  MapController::GetInstance()->InitNearestBerth(q);

#ifdef DEBUG
  // auto end = std::chrono::high_resolution_clock::now();
  // std::chrono::duration<double, std::milli> duration = end - start;
  // std::cerr << "bfs耗时:" << duration.count() << " ms" << std::endl;
#endif

  // 船容积
  scanf("%d", &Boat::boat_capacity);
#ifdef DEBUG
  fprintf(debug_map_file, "船容量：%d\n", Boat::boat_capacity);
#endif
  char okk[100];
  scanf("%s", okk);
#ifdef DEBUG
  fprintf(debug_map_file, "%s", okk);
  fclose(debug_map_file);
#endif
  printf("OK\n");
  fflush(stdout);

  // --------- 其他初始化 ----------
}

// 每帧的数据
void InputController::Input() {
#ifdef DEBUG
  fprintf(debug_command_file,
          "-----------------------------INPUT--------------------------\n");
#endif
  int temp_id = 0;
  int temp_money = 0;
  std::vector<Berth>& berth = MapController::GetInstance()->berth;
  if (scanf("%d%d", &temp_id, &temp_money) == EOF) {
#ifdef DEBUG
    fclose(debug_command_file);

    // 泊位上残留货物数量
    int size = berth.size();
    for (int i = 0; i < size; ++i) {
      std::cerr << i << " 船泊残留货物:" << berth[i].goods_num << std::endl;
      std::cerr << i << " 船泊transport time:" << berth[i].transport_time
                << std::endl;
      for (std::map<int, std::vector<int>>::iterator it = berth[i].path.begin();
           it != berth[i].path.end(); ++it) {
        std::cerr << "船舶 " << i << " 到船舶 " << it->first
                  << " 的距离为:" << it->second.size() << std::endl;
      }
    }

    // 从交货点到各泊位的路径
    for (auto it = Boat::d_path.begin(); it != Boat::d_path.end(); ++it) {
      std::cerr << "交货点(" << it->first.first.x << "," << it->first.first.y
                << ")到泊位" << it->first.second << "距离:" << it->second.size()
                << std::endl;
    }

    // 机器人挣钱统计
    for (std::vector<Robot>::iterator it =
             RentController::GetInstance()->robot.begin();
         it != RentController::GetInstance()->robot.end(); ++it) {
      std::cerr << "机器人 " << it->id_ << " 挣了 " << it->money << std::endl;
    }

    // 船挣钱统计
    for (std::vector<Boat>::iterator it =
             RentController::GetInstance()->boat.begin();
         it != RentController::GetInstance()->boat.end(); ++it) {
      std::cerr << "船 " << it->id_ << " 挣了 " << it->total_money << std::endl;
    }

    // 全局货物统计
    std::cerr << "全局生成货物数量:"
              << MapController::GetInstance()->total_goods_num << std::endl;
    std::cerr << "全局生成筛选货物数量:"
              << MapController::GetInstance()->total_accepted_goods_num
              << std::endl;
    std::cerr << "全局提交货物数量:" << MapController::GetInstance()->pull_num
              << std::endl;

    std::cerr << "全局生成货物金额:"
              << MapController::GetInstance()->total_money << std::endl;
    std::cerr << "全局生成筛选货物金额:"
              << MapController::GetInstance()->total_accepted_goods_money
              << std::endl;
    std::cerr << "全局提交货物金额:" << MapController::GetInstance()->pull_money
              << std::endl;

    std::cerr << "理论得分:"
              << MapController::GetInstance()->pull_money + 25000 -
                     2000 * RentController::GetInstance()->robot.size() -
                     8000 * RentController::GetInstance()->boat.size()
              << std::endl;

#endif
    exit(0);
  }

  // 跳帧
#ifdef DEBUG
  int dis_id = temp_id - id;
  if (dis_id > 1) {
    std::cerr << "跳帧：" << dis_id << std::endl;
  }
  int dis_money = temp_money - money;
  if (id > 1 && dis_money > 0) {
    int flag = true;
    for (std::vector<Boat>::iterator it =
             RentController::GetInstance()->boat.begin();
         it != RentController::GetInstance()->boat.end(); ++it) {
      if (it->path.size() == 1 && it->pos == -1) {
        it->total_money += dis_money;
        std::cerr << "船 " << it->id_ << " 交货，挣了 " << dis_money
                  << std::endl;
        flag = false;
        break;
      }
    }
    if (flag) {
      std::cerr << "违规进账" << std::endl;
    }
  }
#endif

  id = temp_id;
  money = temp_money;
#ifdef DEBUG
  std::cerr << "------------------帧数id: " << id << "------------------"
            << std::endl;
  fprintf(debug_command_file, "帧数 id = %d, 分数 money = %d\n", id, money);
#endif
  // 货物变化
  int num;
  scanf("%d", &num);
#ifdef DEBUG
  fprintf(debug_command_file, "变化货物数量 = %d\n", num);
#endif
  for (int i = 1; i <= num; i++) {
    int x, y, val;
    scanf("%d%d%d", &x, &y, &val);
    ++x;
    ++y;
#ifdef DEBUG
    fprintf(debug_command_file, "货物 %d 信息: x = %d, y = %d, money = %d\n", i,
            x, y, val);
#endif
    auto& nearest_berth = MapController::GetInstance()->nearest_berth;
    if (nearest_berth[x][y] != -1 &&
        MapController::GetInstance()->CanRobotReach(x, y)) {
      if (val > 0) {
#ifdef DEBUG
        MapController::GetInstance()->total_goods_num += 1;  // 全局货物数量增加
        MapController::GetInstance()->total_money += val;  // 全局货物金额增加
#endif
#ifdef GOODS_FILTER
        if (val < berth[nearest_berth[x][y]].goods_manager.value_valve) {
#ifdef DEBUG
          std::cerr << "货物价值过低被抛弃: " << val << std::endl;
#endif
          // 忽略不值钱的货物
          continue;
        }
#endif

#ifdef DEBUG
        MapController::GetInstance()->total_accepted_goods_num +=
            1;  // 全局筛选货物数量增加
        MapController::GetInstance()->total_accepted_goods_money +=
            val;  // 全局筛选货物金额增加
#endif

        Goods* new_goods = new Goods(x, y, val, id);
        berth[nearest_berth[x][y]].goods_manager.PushGoods(new_goods);
        new_goods->area_id = MapController::GetInstance()->FindArea(x * n + y);
      } else if (!val) {
        berth[nearest_berth[x][y]].goods_manager.RemoveGoods(x, y);
      }
    }
  }

  // 机器人实时数据
  std::vector<Robot>& robot = RentController::GetInstance()->robot;
  int(&busy_point)[N][N] = MapController::GetInstance()->busy_point;
  int robot_num;
  scanf("%d", &robot_num);

#ifdef DEBUG
  fprintf(debug_command_file, "机器人 数量:  %d\n", robot_num);
#endif

  for (int i = 0; i < robot_num; i++) {
    int robot_id, temp_goods, x, y;
    scanf("%d%d%d%d", &robot_id, &temp_goods, &x, &y);
    if (i >= static_cast<int>(robot.size())) {
      // 新增机器人
      robot.push_back(Robot(robot_id, temp_goods, ++x, ++y));
      Goods* goods = RentController::GetInstance()->goods.front();
      RentController::GetInstance()->goods.pop();
      robot[robot_id].target_goods = goods;
      goods->robot_id = robot_id;
      robot[robot_id].berth_id =
          MapController::GetInstance()->nearest_berth[goods->x][goods->y];
      berth[robot[robot_id].berth_id].robot.push_back(robot_id);
    } else {
      // 旧机器人更新坐标
      robot[i].x = ++x;
      robot[i].y = ++y;
    }
    ++busy_point[robot[i].x][robot[i].y];
#ifdef DEBUG
    fprintf(debug_command_file, "机器人 %d 信息: goods = %d, x = %d, y = %d\n",
            robot_id, temp_goods, robot[i].x, robot[i].y);

#endif
    // 放置成功港口货物加一
    if (robot[i].pre_goods - temp_goods == 1) {
      ++berth[robot[i].berth_id].goods_num;
      berth[robot[i].berth_id].goods.push_back(robot[i].goods_money);
#ifdef DEBUG
      if (robot[i].berth_id !=
          MapController::GetInstance()->location_to_berth_id[Location(x, y)]) {
        std::cerr << "泊位货物数量增加错了" << std::endl;
      }
      // 全局放置数量增加
      MapController::GetInstance()->pull_num += 1;
      MapController::GetInstance()->pull_money += robot[i].goods_money;
      robot[i].money += robot[i].goods_money;
      std::cerr << robot[i].berth_id << " 泊位更新货物数量："
                << berth[robot[i].berth_id].goods_num << std::endl;
#endif
    } else if (robot[i].goods && !temp_goods) {
#ifdef DEBUG
      // 假get
      std::cerr << "假get" << std::endl;
#endif
    }
    robot[i].goods = temp_goods;
    robot[i].pre_goods = temp_goods;
  }

  // 船的实时数据
  int boat_num;
  scanf("%d", &boat_num);

#ifdef DEBUG
  fprintf(debug_command_file, "boat num:  %d\n", boat_num);
#endif
  std::vector<Boat>& boat = RentController::GetInstance()->boat;
  for (int i = 0; i < boat_num; i++) {
    int boat_id, goods_num, x, y, direction, temp_status;
    scanf("%d%d%d%d%d%d\n", &boat_id, &goods_num, &x, &y, &direction,
          &temp_status);
    if (i >= static_cast<int>(RentController::GetInstance()->boat.size())) {
      boat.push_back(
          Boat(boat_id, goods_num, ++x, ++y, direction, temp_status));
    } else {
      // 判断船的指令是否执行成功
      if (boat[i].x != (x + 1) || boat[i].y != (y + 1) ||
          boat[i].direction != direction) {
        // 指令执行成功
        boat[i].RemoveFirst();
        ++boat[i].total_transport_time;
        boat[i].stuck_times = 0;
      } else if (temp_status == BOAT_STATUS_MOVING &&
                 boat[i].status != BOAT_STATUS_RESTORING &&
                 boat[i].path.size()) {
        ++boat[i].stuck_times;
#ifdef DEBUG
        std::cerr << boat_id << " 船堵塞，次数：" << boat[i].stuck_times
                  << std::endl;
#endif
      }

      boat[i].x = ++x;
      boat[i].y = ++y;
      int dis_goods_num = goods_num - boat[i].num;
      if (dis_goods_num > 0 && boat[i].pos != -1) {
        berth[boat[i].pos].goods_num -= dis_goods_num;
        // 船装货，钱增加
        while (dis_goods_num) {
          boat[i].money +=
              berth[boat[i].pos].goods[berth[boat[i].pos].first_index++];
          --dis_goods_num;
        }
#ifdef DEBUG
        if (boat[i].pos != MapController::GetInstance()
                               ->location_to_berth_id[Location(x, y)]) {
          std::cerr << "泊位数量减少错了" << std::endl;
        }
        std::cerr << boat[i].pos << " 泊位更新货物数量："
                  << berth[boat[i].pos].goods_num << std::endl;
#endif
      } else if (dis_goods_num < 0) {
        // 装完货，将行驶路程置零
        boat[i].total_transport_time = 0;
      }
      boat[i].num = goods_num;
      boat[i].direction = direction;
    }
    // // 到达泊位入队
    // if (temp_status != boat[i].status && temp_status == 0 &&
    //     boat[i].pos != -1) {
    //   berth[boat[i].pos].q_boat.push(i);
    // }

    boat[i].status = temp_status;
#ifdef DEBUG
    fprintf(debug_command_file,
            "boat %d info: goods_num = %d, x = %d, y = %d, direction = %s, "
            "status = %d\n",
            boat_id, goods_num, x, y, Boat::dir_str[direction], temp_status);
#endif
  }
  char okk[100];
  scanf("%s", okk);
#ifdef DEBUG
  fprintf(debug_command_file, "%s\n", okk);
#endif
}
