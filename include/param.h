#ifndef PARAM_H_
#define PARAM_H_
// #define DEBUG
#define FACE_MAP  // 面向地图
// #define TEST_ASTAR

#define MONEY_FIRST  // 金钱贪心 关了就是 距离贪心

// #define FILLBERTH  // 填充泊位 打开就是填充， 关闭就是不填充

// 货物生存周期
#define LIFETIME 1000
// 地图长度
const int n = 200;
const int N = 210;

// 决策类型
#define DECISION_TYPE_ROBOT_LBOT 0
#define DECISION_TYPE_ROBOT_MOVE 1
#define DECISION_TYPE_ROBOT_GET 2
#define DECISION_TYPE_ROBOT_PULL 3

#define DECISION_TYPE_BOAT_LBOAT 4
#define DECISION_TYPE_BOAT_DEPT 5
#define DECISION_TYPE_BOAT_BERTH 6
#define DECISION_TYPE_BOAT_ROT 7
#define DECISION_TYPE_BOAT_SHIP 8

#define DECISION_ROBOT_RIGHT 0
#define DECISION_ROBOT_LEFT 1
#define DECISION_ROBOT_UP 2
#define DECISION_ROBOT_DOWN 3

#define DECISION_BOAT_ROT_CLOCKWISE 0
#define DECISION_BOAT_ROT_COUNTERCLOCKWISE 1
#define DECISION_BOAT_SHIP 2  // 直行

#define BOAT_DIRECTION_RIGHT 0
#define BOAT_DIRECTION_LEFT 1
#define BOAT_DIRECTION_UP 2
#define BOAT_DIRECTION_DOWN 3

#define BOAT_STATUS_MOVING 0
#define BOAT_STATUS_RESTORING 1
#define BOAT_STATUS_LOADING 2

// A*算法深度
// #define CHANGE_CLOSED_GOODS  // 开关 A*是否切换更近的货物

/*
 * 是否开启货物筛选机制
 * 开启后将按照 GOODS_VALUE_VALVE 筛选货物
 * 当货物数量超过 GOODS_FILTER_VALVE_NUM 将会按照 VALUEABLE_GOODS_VALVE 筛选货物
 */
#define GOODS_FILTER  // 开关 货物过滤机制

class DynamicParam {
 public:
  static DynamicParam*& GetInstance();

  int GetTolerantTime() { return tolerant_time_; }
  int GetTolerantLeaveTime() { return tolerant_leave_time_; }
  int GetGoodsValueValve() { return goods_value_valve_; }
  int GetGoodsFilterValveNum() { return goods_filter_valve_num_; }
  int GetValueableGoodsValve() { return valueable_goods_valve_; }
  int GetFinalTolerantTime() { return final_tolerant_time_; }
  int GetBusyValve() { return busy_valve_; }
  int GetBoatCapacityReduce() { return boat_capacity_reduce_; }
  int GetMaxRobotNum() { return max_robot_num_; }
  int GetMaxBoatNum() { return max_boat_num_; }
  bool GetMultipleAstar() { return multiple_astar_; }
  double GetAvrMoneyDifferential() { return avr_money_differential_; }
  int GetFindNeighborMaxRobot() { return find_neighbor_max_robot_; }

  void SetTolerantTime(int tolerant_time) { tolerant_time_ = tolerant_time; }
  void SetTolerantLeaveTime(int tolerant_leave_time) {
    tolerant_leave_time_ = tolerant_leave_time;
  }
  void SetGoodsValueValve(int goods_value_valve) {
    goods_value_valve_ = goods_value_valve;
  }
  void SetGoodsFilterValveNum(int goods_filter_valve_num) {
    goods_filter_valve_num_ = goods_filter_valve_num;
  }
  void SetValueableGoodsValve(int valueable_goods_valve) {
    valueable_goods_valve_ = valueable_goods_valve;
  }
  void SetFinalTolerantTime(int final_tolerant_time) {
    final_tolerant_time_ = final_tolerant_time;
  }
  void SetBusyValve(int busy_valve) { busy_valve_ = busy_valve; }
  void SetBoatCapacityReduce(int boat_capacity_reduce) {
    boat_capacity_reduce_ = boat_capacity_reduce;
  }
  void SetMaxRobotNum(int max_robot_num) { max_robot_num_ = max_robot_num; }
  void SetMaxBoatNum(int max_boat_num) { max_boat_num_ = max_boat_num; }
  void SetMultipleAstar(bool multiple_astar) {
    multiple_astar_ = multiple_astar;
  }
  void SetAvrMoneyDifferential(double avr_money_differential) {
    avr_money_differential_ = avr_money_differential;
  }
  void SetFindNeighborMaxRobot(bool find_neighbor_max_robot) {
    find_neighbor_max_robot_ = find_neighbor_max_robot;
  }

 private:
  int tolerant_time_ = 20;           // 参数 机器人找货物容忍步数
  int tolerant_leave_time_ = 40;     // 参数 船最后走可容忍时间的
  int goods_value_valve_ = 15;       // 参数 货物筛选域值
  int goods_filter_valve_num_ = 60;  // 参数 货物筛选数量域值
  int valueable_goods_valve_ = 60;   // 参数 贵重货物域值
  int final_tolerant_time_ = 200;    // 参数 最后冲刺阶段的容忍时间
  int busy_valve_ = 5;               // 参数 判断该点拥堵的域值
  int boat_capacity_reduce_ = 0;     // 参数 改变船容量
  int max_robot_num_ = 13;           // 参数 机器人最大数量
  int max_boat_num_ = 1;             // 参数 船舶最大数量
  double avr_money_differential_ = 0;  // 参数 寻找邻居货物时更新权重的差值
  int find_neighbor_max_robot_ = 2;  // 参数 寻找邻居货物的机器人数量限制
  bool multiple_astar_ = false;  // 参数 一帧可多次astar吗？

  static DynamicParam* instance_;

  // 私有构造函数，防止直接实例化
  DynamicParam() {}
};
#endif
