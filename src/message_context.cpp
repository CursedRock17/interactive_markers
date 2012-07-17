/*
 * Copyright (c) 2011, Willow Garage, Inc.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of the Willow Garage, Inc. nor the names of its
 *       contributors may be used to endorse or promote products derived from
 *       this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
 * LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGE.
 *
 * Author: David Gossow
 */

#include "interactive_markers/detail/message_context.h"

#include <visualization_msgs/InteractiveMarkerInit.h>
#include <visualization_msgs/InteractiveMarkerUpdate.h>

#include <boost/make_shared.hpp>

#define DBG_MSG( ... ) ROS_INFO_NAMED( "im_client", __VA_ARGS__ );

namespace interactive_markers
{

template<class MsgT>
MessageContext<MsgT>::MessageContext(
    const tf::Transformer& tf,
    const std::string& target_frame,
    const typename MsgT::ConstPtr& _msg)
: tf_(tf)
, target_frame_(target_frame)
{
  // copy message, as we will be modifying it
  msg = boost::make_shared<MsgT>( *_msg );
  init();
}

template<class MsgT>
template<class MsgVecT>
void MessageContext<MsgT>::getTfTransforms( MsgVecT msg_vec, std::list<size_t>& indices )
{
  std::list<size_t>::iterator idx_it;
  for ( idx_it = indices.begin(); idx_it != indices.end(); )
  {
    try
    {
      // get transform
      tf::StampedTransform transform;
      std_msgs::Header& header = msg_vec[ *idx_it ].header;
      tf_.lookupTransform( target_frame_, header.frame_id, header.stamp, transform );
      // transform message into target frame
      tf::Pose pose;
      tf::poseMsgToTF( msg_vec[ *idx_it ].pose, pose );
      pose = transform * pose;
      // store transformed pose in original message
      tf::poseTFToMsg( pose, msg_vec[ *idx_it ].pose );
      msg_vec[ *idx_it ].header.frame_id = target_frame_;

      indices.erase(idx_it++);
    }
    catch ( tf::ExtrapolationException& e )
    {
      // skip and wait for more up-to-date
      ++idx_it;
    }
    // all other exceptions need to be handled outside
  }
}

template<class MsgT>
bool MessageContext<MsgT>::isReady()
{
  return open_marker_idx_.empty() && open_pose_idx_.empty();
}

template<>
void MessageContext<visualization_msgs::InteractiveMarkerUpdate>::init()
{
  // mark all transforms as being missing
  for ( size_t i=0; i<msg->markers.size(); i++ )
  {
    open_marker_idx_.push_back( i );
  }
  for ( size_t i=0; i<msg->poses.size(); i++ )
  {
    open_pose_idx_.push_back( i );
  }
}

template<>
void MessageContext<visualization_msgs::InteractiveMarkerInit>::init()
{
  // mark all transforms as being missing
  for ( size_t i=0; i<msg->markers.size(); i++ )
  {
    open_marker_idx_.push_back( i );
  }
}

template<>
void MessageContext<visualization_msgs::InteractiveMarkerUpdate>::getTfTransforms( )
{
  getTfTransforms( msg->markers, open_marker_idx_ );
  getTfTransforms( msg->poses, open_pose_idx_ );
}

template<>
void MessageContext<visualization_msgs::InteractiveMarkerInit>::getTfTransforms( )
{
  getTfTransforms( msg->markers, open_marker_idx_ );
}

// explicit template
template class MessageContext<visualization_msgs::InteractiveMarkerUpdate>;
template class MessageContext<visualization_msgs::InteractiveMarkerInit>;


}

