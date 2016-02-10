/* celtic_knots - generate and animate random Celtic knots
 * Copyright (C) 2008 Lewis Van Winkle
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include "cknot.hpp"

#include <algorithm>
#include <cmath>
#include <map>
#include <set>
#include <vector>

namespace CKnot
{

    vec2 vec2::operator+(const vec2& rhs) const
    {
        vec2 ret(*this);
        ret.x += rhs.x;
        ret.y += rhs.y;
        return ret;
    }

    vec2 vec2::operator-(const vec2& rhs) const
    {
        vec2 ret(*this);
        ret.x -= rhs.x;
        ret.y -= rhs.y;
        return ret;
    }

    vec2 vec2::operator*(double rhs) const
    {
        vec2 ret(*this);
        ret.x *= rhs;
        ret.y *= rhs;
        return ret;
    }

    vec2 vec2::operator-() const
    {
        vec2 ret(*this);
        ret.x *= -1.0;
        ret.y *= -1.0;
        return ret;
    }

    bool vec2::operator==(const vec2& rhs) const
    {
        return x == rhs.x && y == rhs.y;
    }

    bool vec2::operator<(const vec2& rhs) const
    {
        if (x == rhs.x)
            return y < rhs.y;
        else
            return x < rhs.x;
    }


    double vec2::GetAngle() const
    {
        return std::atan2(y, x);
    }


    double vec2::GetLength() const
    {
        const double x2 = x * x;
        const double y2 = y * y;
        return std::sqrt(x2 + y2);
    }


    double Stroke::GetAngle() const
    {
        vec2 angle = b - a;
        return angle.GetAngle();
    }


    double Stroke::GetLength() const
    {
        const vec2 len = b - a;
        return len.GetLength();
    }


    vec2 Stroke::GetMid() const
    {
        vec2 mid = b - a;
        mid.x /= 2.0;
        mid.y /= 2.0;
        return a + mid;
    }


    Art::Art(const SplineVector& sv, const ZVector& zv)
        :mThreads(sv), mZs(zv)
    {
    }


    Art::~Art()
    {
        for (SplineVector::const_iterator it = mThreads.begin(); it != mThreads.end(); ++it)
            delete *it;
        mThreads.clear();

        for (ZVector::const_iterator it = mZs.begin(); it != mZs.end(); ++it)
            delete *it;
        mZs.clear();
    }


    const Art::Thread* Art::GetThread(size_t index) const
    {
        assert(index < GetThreadCount());
        return mThreads[index];
    }


    const Art::Z* Art::GetZ(size_t index) const
    {
        assert(index < GetThreadCount());
        return mZs[index];
    }


    namespace
    {
        struct Junction;

        enum Dir {fLeft, fRight, bLeft, bRight};

        Dir BounceDir(Dir d)
        {
            switch (d)
            {
                case fLeft: return bLeft;
                case fRight: return bRight;
                case bRight: return fRight;
                case bLeft: return fLeft;
                default: assert(0);
            }
        }

        Dir CrossDir(Dir d)
        {
            switch (d)
            {
                case fLeft: return bRight;
                case fRight: return bLeft;
                case bRight: return fLeft;
                case bLeft: return fRight;
                default: assert(0);
            }
        }

        Dir GlanceDir(Dir d)
        {
            switch (d)
            {
                case fLeft: return fRight;
                case fRight: return fLeft;
                case bRight: return bLeft;
                case bLeft: return bRight;
                default: assert(0);
            }
        }

        ///Defines a node in the middle of a stroke.
        struct Node
        {
            Node(vec2 mid, Dir dir):mid(mid), dir(dir), type(Cross)
            {}

            bool operator<(const Node& rhs) const
            {
                if (mid == rhs.mid)
                    return dir < rhs.dir;
                else
                    return mid < rhs.mid;
            }

            vec2 mid; ///<Crossing point.
            Dir dir;
            StrokeType type;

            vec2 normal;
            Junction* left;
            Junction* right;
        };

        typedef std::list<Node> NodeList;
        typedef std::set<Node> NodeSet;

        ///Compares two vectors based on their angle from a point.
        struct VecAngleComp
        {
            vec2 point; ///<Points to compare around.
            VecAngleComp(vec2 point):point(point){}

            bool operator()(const vec2& lhs, const vec2& rhs)
            {
                return Stroke(point, lhs).GetAngle() < Stroke(point, rhs).GetAngle();
            }
        };

        ///Defines a junction that hooks up several strokes.
        struct Junction
        {
            vec2 position;
            std::list<vec2> mids; ///<A list of each mid point on each stroke connecting to this junction.

            vec2 FindNext(vec2 v, bool clockwise)
            {
                vec2 ret = v;

                std::list<vec2>::const_iterator it = std::find(mids.begin(), mids.end(), v);

                if (it != mids.end())
                {
                    if (clockwise)
                    {
                        ++it;

                        if (it == mids.end())
                            it = mids.begin();

                        ret = *it;
                    }
                    else
                    {
                        if (it == mids.begin())
                            it = mids.end();

                        --it;

                        ret = *it;
                    }
                }

                return ret;
            }
        };

        typedef std::map<vec2, Junction*> JunctionMap;

        struct Graph
        {
            NodeSet unused; ///<Unused nodes.
            JunctionMap junctions; ///<Junction position lookup.

            Graph(const StrokeList& strokes)
            {
                for (StrokeList::const_iterator it = strokes.begin(); it != strokes.end(); ++it)
                {
                    const vec2 mid = it->GetMid();

                    vec2 normal = it->b - it->a;
                    normal = vec2(-normal.y, normal.x);

                    //Create junctions.
                    const vec2 a = it->a;
                    Junction* ja;
                    if (junctions.count(a))
                        ja = junctions[a];
                    else
                        junctions[a] = (ja = new Junction);
                    ja->position = a;
                    ja->mids.push_back(mid);


                    const vec2 b = it->b;
                    Junction* jb;
                    if (junctions.count(b))
                        jb = junctions[b];
                    else
                        junctions[b] = (jb = new Junction);
                    jb->position = b;
                    jb->mids.push_back(mid);


                    {//Remember unused nodes.
                        Node n = Node(mid, fLeft);
                        n.normal = normal;
                        n.left = ja;
                        n.right = jb;
                        n.type = it->type;
                        unused.insert(n);

                        n.dir = fRight;
                        unused.insert(n);

                        n.dir = bLeft;
                        unused.insert(n);

                        n.dir = bRight;
                        unused.insert(n);
                    }


                    {//Sort junctions.
                        for (JunctionMap::const_iterator it = junctions.begin(); it != junctions.end(); ++it)
                            it->second->mids.sort(VecAngleComp(it->second->position));
                    }
                }

            }

            ~Graph()
            {
                for (JunctionMap::const_iterator it = junctions.begin(); it != junctions.end(); ++it)
                    delete it->second;
                junctions.clear();
            }
        };
    }


    AutoArt CreateThread(const StrokeList& strokes)
    {
        const double rot = std::atan(1.0); //45 degrees.

        Graph g(strokes);

        Art::SplineVector ret;
        Art::ZVector retZs;

        NodeSet unusedUp; //Stores cross type nodes that have been crossed but are unused.

        while (g.unused.size() || unusedUp.size())
        {
            std::vector<vec2> thread;
            thread.reserve(g.unused.size() + 1);

            std::vector<vec2> angles;
            angles.reserve(thread.capacity());

            std::vector<double> zs;
            zs.reserve(thread.capacity());

            bool up = false; //Over or under?

            //Get the starting node to draw. If there are nodes that have been crossed already, we must use one of them.
            Node cur = unusedUp.size() ? *unusedUp.begin() : *g.unused.begin();

            if (unusedUp.size())
                up = true;

            while(true)
            {
                zs.push_back(up ? 1.0 : 0.0);

                if (up == false && cur.type == Cross)
                {
                    //If an unused node is crossed, remember that we go under and it should go over.
                    Node above = cur;
                    above.dir = GlanceDir(above.dir); //Get the crossing node (at 90 degrees).
                    if (g.unused.count(above))
                    {
                        //Mark the node in each direction.
                        unusedUp.insert(above);

                        above.dir = CrossDir(above.dir);
                        assert(g.unused.count(above));
                        unusedUp.insert(above);
                    }
                }

                //At this point, the top of the loop, cur is the entry node.
                //Mark the entry direction node as used.
                unusedUp.erase(cur);
                assert(g.unused.count(cur));
                g.unused.erase(cur);

                //Set the from direction for the node.
                Junction* j = (cur.dir == fLeft || cur.dir == bLeft) ? cur.left : cur.right;

                //Switch to the exit direction.
                switch (cur.type)
                {
                    case Bounce: cur.dir = BounceDir(cur.dir); break;
                    case Cross: cur.dir = CrossDir(cur.dir); break;
                    case Glance: cur.dir = GlanceDir(cur.dir); break;
                    default: assert(0);
                }

                //Mark the exit direction as used.
                unusedUp.erase(cur);
                assert(g.unused.count(cur));
                g.unused.erase(cur);

                const bool isFront = (cur.dir == fLeft || cur.dir == fRight);
                const bool isLeft = (cur.dir == fLeft || cur.dir == bLeft);

                //Add the thread point and normals.
                if (cur.type == Cross)
                {
                    up = !up; //If the threads cross then we weave up and down.

                    thread.push_back(cur.mid);

                    vec2 normal = cur.normal;
                    double t = normal.GetAngle();
                    switch (cur.dir)
                    {
                        case fLeft: t += rot * 1; break;
                        case bLeft: t += rot * 3; break;
                        case bRight: t -= rot * 3; break;
                        case fRight: t -= rot; break;
                        default: assert(0);
                    }

                    normal = vec2(std::cos(t), std::sin(t)) * cur.normal.GetLength();
                    normal = normal * 1.3;

                    angles.push_back(normal);
                }
                else if (cur.type == Glance)
                {
                    //How far to offset the glancing blow.
                    const vec2 offset = cur.normal * (isFront ? 0.25 : -0.25);
                    thread.push_back(cur.mid + offset);

                    //Create normal.
                    vec2 normal = (cur.left->position - cur.right->position) * (isLeft ? 0.3 : -0.3);
                    angles.push_back(normal);
                }
                else if (cur.type == Bounce)
                {
                    //How far to offset the bouncing blow.
                    vec2 offset = (cur.left->position - cur.right->position) * (isLeft ? 0.25 : -0.25);
                    thread.push_back(cur.mid + offset);

                    //Create normal.
                    const vec2 normal = cur.normal * (isFront ? 0.3 : -0.3);
                    angles.push_back(normal);
                }

                if (cur.type != Bounce)
                    j = (j == cur.right) ? cur.left : cur.right; //The next junction is the one that we weren't just at.

                const bool clockwise = cur.dir == fLeft || cur.dir == bRight;
                const vec2 next = j->FindNext(cur.mid, clockwise);

                const Dir dir = clockwise ? fRight : bRight; //Attempt entry from the right first.

                Node n(next, dir);
                NodeSet::const_iterator it = g.unused.find(n);

                if (it == g.unused.end())
                {
                    //Try the other direction.
                    n.dir = CrossDir(dir);
                    it = g.unused.find(n);

                    if (it == g.unused.end())
                        break; //Nowhere to go. Start next thread.

                    //Found good.
                    cur = *it;
                }
                else
                {
                    //Fill in the pointers.
                    cur = *it;

                    //Now see if the entry is from the correct direction.
                    if (cur.right != j)
                    {
                        cur.dir = CrossDir(cur.dir);
                        if (!g.unused.count(cur))
                            break; //Nowhere to go. Start next thread.
                    }
                }
            }


            const size_t frames = thread.size(); ///<Count up for each node visited.
            double* xs = new double[frames + 1];
            for (size_t i = 0; i < frames; ++i)
                xs[i] = double(i) / double(frames);
            xs[frames] = 1.0;

            thread.push_back(thread.front());
            angles.push_back(angles.front());
            zs.push_back(zs.front());

            ret.push_back(new Art::Thread(xs, &thread.front(), &angles.front(), frames + 1, true));
            retZs.push_back(new Art::Z(xs, &zs.front(), frames + 1, true));

            delete[] xs;
        }

        return AutoArt(new Art(ret, retZs));
    }

}
