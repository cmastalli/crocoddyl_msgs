///////////////////////////////////////////////////////////////////////////////
// BSD 3-Clause License
//
// Copyright (C) 2020-2023, Heriot-Watt University, University of Oxford
// Copyright note valid unless otherwise stated in individual files.
// All rights reserved.
///////////////////////////////////////////////////////////////////////////////

#ifndef CROCODDYL_MSG_SOLVER_STATISTICS_SUBSCRIBER_H_
#define CROCODDYL_MSG_SOLVER_STATISTICS_SUBSCRIBER_H_

#include <mutex>
#include <ros/node_handle.h>

#include "crocoddyl_msgs/SolverStatistics.h"

namespace crocoddyl_msgs {

class SolverStatisticsRosSubscriber {
public:
  /**
   * @brief Initialize the solver statistic subscriber
   *
   * @param[in] topic  Topic name
   */
  SolverStatisticsRosSubscriber(
      const std::string &topic = "/crocoddyl/solver_statistics")
      : spinner_(2), has_new_msg_(false), is_processing_msg_(false),
        last_msg_time_(0.) {
    ros::NodeHandle n;
    sub_ = n.subscribe<crocoddyl_msgs::SolverStatistics>(
        topic, 1, &SolverStatisticsRosSubscriber::callback, this,
        ros::TransportHints().tcpNoDelay());
    spinner_.start();
    std::cout << "Ready to subscribe to solver statistics" << std::endl;
  }
  ~SolverStatisticsRosSubscriber() = default;

  /**
   * @brief Get the latest solver statistic
   *
   * @return  A tuple with the number of iterations, total time, solve time,
   * cost, regularization, step legth, dynamic, equality and inequality
   * feasibilities.
   */
  std::tuple<std::size_t, double, double, double, double, double, double,
             double, double>
  get_solver_statistics() {
    // start processing the message
    is_processing_msg_ = true;
    std::lock_guard<std::mutex> guard(mutex_);
    iterations_ = msg_.iterations;
    total_time_ = msg_.total_time;
    solve_time_ = msg_.solve_time;
    cost_ = msg_.cost;
    regularization_ = msg_.regularization;
    step_length_ = msg_.step_length;
    dynamic_feasibility_ = msg_.dynamic_feasibility;
    equality_feasibility_ = msg_.equality_feasibility;
    inequality_feasibility_ = msg_.inequality_feasibility;
    // finish processing the message
    is_processing_msg_ = false;
    has_new_msg_ = false;
    return {iterations_,
            total_time_,
            solve_time_,
            cost_,
            regularization_,
            step_length_,
            dynamic_feasibility_,
            equality_feasibility_,
            inequality_feasibility_};
  }

  /**
   * @brief Indicate whether we have received a new message
   */
  bool has_new_msg() const { return has_new_msg_; }

private:
  ros::AsyncSpinner spinner_;
  ros::Subscriber sub_; //!< ROS subscriber
  std::mutex mutex_;    //!< Mutex to prevent race condition on callback
  crocoddyl_msgs::SolverStatistics msg_; //!< Solver statistics message
  bool has_new_msg_;       //!< Indcate when a new message has been received
  bool is_processing_msg_; //!< Indicate when we are processing the message
  double last_msg_time_; //!< Last message time needed to ensure each message is
                         //!< newer
  std::size_t iterations_;
  double total_time_;
  double solve_time_;
  double cost_;
  double regularization_;
  double step_length_;
  double dynamic_feasibility_;
  double equality_feasibility_;
  double inequality_feasibility_;
  void callback(const crocoddyl_msgs::SolverStatisticsConstPtr &msg) {
    if (!is_processing_msg_) {
      double t = msg->stamp.toNSec();
      // Avoid out of order arrival and ensure each message is newer (or equal
      // to) than the preceeding:
      if (last_msg_time_ <= t) {
        std::lock_guard<std::mutex> guard(mutex_);
        msg_ = *msg;
        has_new_msg_ = true;
        last_msg_time_ = t;
      } else {
        ROS_WARN_STREAM("Out of order message. Last timestamp: "
                        << std::fixed << last_msg_time_
                        << ", current timestamp: " << t);
      }
    }
  }
};

} // namespace crocoddyl_msgs

#endif // CROCODDYL_MSG_SOLVER_STATISTICS_SUBSCRIBER_H_
