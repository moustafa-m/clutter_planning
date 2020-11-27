#include "gazebo_geometries_plugin/geometries_plugin.h"

using namespace gazebo;

GeometriesPlugin::GeometriesPlugin()
    : WorldPlugin()
{
    get_geometry_srv = nh_.advertiseService("/gazebo/get_geometry", &GeometriesPlugin::getGeometrySrv, this);
}

GeometriesPlugin::~GeometriesPlugin()
{

}

void GeometriesPlugin::Load(physics::WorldPtr _world, sdf::ElementPtr _sdf)
{
    if (!ros::isInitialized())
    {
        ROS_FATAL("Gazebo ROS node not initialised!");
        return;
    }

    world_ = _world;
    ROS_INFO("%sGeometriesPlugin: Plugin loaded!", GREEN);
}

bool GeometriesPlugin::getGeometrySrv(gazebo_geometries_plugin::geometry::Request &req, gazebo_geometries_plugin::geometry::Response &res)
{
    physics::ModelPtr model;
    bool find_link = false;

    // first check if user is asking for a link's geometry
    std::string::size_type pos = req.model_name.find("_link");
    if (pos != std::string::npos)
    {
        find_link = true;

        ROS_INFO("%sGeometriesPlugin: Requests for link [%s] recieved!", GREEN, req.model_name.c_str());
        std::string name = req.model_name.substr(0, pos);
        ROS_INFO("%sGeometriesPlugin: Looking for parent [%s]", GREEN, name.c_str());
        model = world_->ModelByName(name);
    }
    else
    {
        model = world_->ModelByName(req.model_name);
        ROS_INFO("%sGeometriesPlugin: Looking for [%s]", GREEN, req.model_name.c_str());
    }
    
    if (!model)
    {
        ROS_ERROR("GeometriesPlugin: Unable to find [%s]! Model does not exist", req.model_name.c_str());

        res.message = "Error, model does not exist!";
        geometry_msgs::Vector3 size;
        // size.x = size.y = size.z = NAN;
        // res.min_bounds = res.max_bounds = size;
        return false;
    }

    ignition::math::Box collision_box;
    geometry_msgs::Vector3 min, max, centre, dimensions;
    ROS_INFO("%sGeometriesPlugin: Found %d child links for [%s]", GREEN, model->GetChildCount(), req.model_name.c_str());
    
    // loop through parent model child links
    for (unsigned int i = 0 ; i < model->GetChildCount(); i ++)
    {
        gazebo::physics::LinkPtr body = boost::dynamic_pointer_cast<gazebo::physics::Link>(model->GetChild(i));

        if (body)
        {
            std::size_t npos = std::string::npos;
            // NOTE: kinova model has multiple child links, each with multiple collision geometry, default gazebo models have one
            // child link with multiple collision geometries
            std::string::size_type pos = req.model_name.find("link_");
            std::string link_name = req.model_name.substr(pos+5, req.model_name.length());
            if (req.model_name.find("j2s7s300") != npos || req.model_name.find("j2s6s300") != npos || req.model_name.find("j2n6s300") != npos)
            {
                link_name = req.model_name + "_collision";
            }
            
            for (unsigned int j = 0; j < body->GetChildCount() ; j++)
            {
                // check if user is asking for a specific link
                if (!find_link)
                {
                    gazebo::physics::CollisionPtr geom = boost::dynamic_pointer_cast<gazebo::physics::Collision>(body->GetChild(j));
                    collision_box = geom->CollisionBoundingBox();
                    
                    min.x = collision_box.Min().X(); min.y = collision_box.Min().Y(); min.z = collision_box.Min().Z();
                    max.x = collision_box.Max().X(); max.y = collision_box.Max().Y(); max.z = collision_box.Max().Z();
                    centre.x = collision_box.Center().X(); centre.y = collision_box.Center().Y(); centre.z = collision_box.Center().Z();
                    dimensions.x = collision_box.XLength(); dimensions.y = collision_box.YLength(); dimensions.z = collision_box.ZLength();

                    res.name.push_back(req.model_name + "_" + body->GetChild(j)->GetName());
                    res.min_bounds.push_back(min);
                    res.max_bounds.push_back(max);
                    res.centre.push_back(centre);
                    res.dimensions.push_back(dimensions);
                }
                else if (body->GetChild(j)->GetName().compare(link_name) == 0)
                {
                    ROS_INFO("%sGeometriesPlugin: Found [%s]!", GREEN, link_name.c_str());
                    gazebo::physics::CollisionPtr geom = boost::dynamic_pointer_cast<gazebo::physics::Collision>(body->GetChild(j));
                    collision_box = geom->CollisionBoundingBox();

                    min.x = collision_box.Min().X(); min.y = collision_box.Min().Y(); min.z = collision_box.Min().Z();
                    max.x = collision_box.Max().X(); max.y = collision_box.Max().Y(); max.z = collision_box.Max().Z();
                    centre.x = collision_box.Center().X(); centre.y = collision_box.Center().Y(); centre.z = collision_box.Center().Z();
                    dimensions.x = collision_box.XLength(); dimensions.y = collision_box.YLength(); dimensions.z = collision_box.ZLength();

                    res.name.push_back(req.model_name);
                    res.min_bounds.push_back(min);
                    res.max_bounds.push_back(max);
                    res.centre.push_back(centre);
                    res.dimensions.push_back(dimensions);
                    break;
                }
                
            }
        }
    }
    
    res.message = "GeomtriesPlugin: Model found!";
    ROS_INFO("%sGeometriesPlugin: Obtained collision geometries for [%s]", GREEN, req.model_name.c_str());

    return true;
}