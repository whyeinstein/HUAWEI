#ifndef ROBOT_H_
#define ROBOT_H_

#include "astar.h"
#include "goods.h"
#include "nextpoint.h"

// 机器人
struct Robot {
  int id_;

  int x, y;

  // 是否携带货物
  int goods;

  //保存上一刻的货物状态
  int pre_goods = 0;

  // 手里拿的物品的价值
  int goods_money;

  // 目标港口
  int berth_id;

  // 冲刺标志
  bool is_sprint = false;

  // 所处区号
  int area_id;

#ifdef DEBUG
  int money = 0;
#endif
  Robot() = default;
  Robot(int &id, int &goods, int &startX, int &startY);

  Goods *target_goods = nullptr;
  std::vector<Location> path;

  // 寻找目标货物
  bool FindTargetGoods();

  // 删除path的第一个点
  void RemoveFirst();

  // 在头位置添加一个点
  void AddFirst(int x, int y);

  /*
   * 判断哪个机器人优先级高
   * @ret - 1 第一个优先级高
   * @ret - 2 第二个优先级高
   * 同等优先级默认第一个优先级高
   *
   * --- 优先级策略 ---
   * 优先级高到低：
   * - 都有货物价值高优先
   * - 有一个有货物，没货物优先
   * - 都没货物，目标货物生命周期少的优先
   * - 有人没目标货物，有目标货物的优先
   * - 都没目标货物，先判断的优先
   */
  //
  static int JudgePriority(Robot *first, Robot *second);

  //拿到货物后寻找港口
  void FindBerth(int start_x, int start_y);

  /*
   * 机器人是否拦路
   * @ret next_points的下标
   */
  int IsBlock(std::vector<NextPoint> &next_points);

  /*
   * 让路
   * @ret 让路方向： DIR 数组下标
   * @bref 只考虑空闲位
   */
  int GetAway(std::vector<NextPoint> &next_points, int ignore_id,
              std::vector<int> &not_move_id);

  // 分区规划
  bool ZonePlan();

  // 偷邻居的货物
  void FindNeighborGoods();

  /*
   * 找路径
   */
  bool FindPath(Goods *&find_goods);

  // 更换泊位
  void ChangeBerth(int new_berth_id);

  // 设置冲刺
  void SetDash(const int &target_berth);
};

#endif