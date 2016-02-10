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

#ifndef __CKNOT_HPP__
#define __CKNOT_HPP__

#include <list>
#include <memory>
#include <vector>
#include "spline.hpp"

namespace CKnot
{

    struct vec2
    {
        double x, y;

        vec2 (double x, double y):x(x), y(y){}
        vec2 ():x(0), y(0){}

        vec2 operator+(const vec2& rhs) const;
        vec2 operator-(const vec2& rhs) const;
        vec2 operator*(double rhs) const;
        vec2 operator-() const;
        bool operator==(const vec2& rhs) const;
        bool operator<(const vec2& rhs) const;

        double GetAngle() const; ///<Returns the angle of the vector.
        double GetLength() const; ///<Returns the length of the vector.
    };

    enum StrokeType {Cross, Bounce, Glance};

    ///Defines a line of the graph for defining knots. A and B are both junctions.
    struct Stroke
    {
        vec2 a, b;
        StrokeType type;

        Stroke (vec2 a, vec2 b): a(a), b(b), type(Cross){}
        Stroke (vec2 a, vec2 b, StrokeType st): a(a), b(b), type(st){}

        double GetAngle() const; ///<Returns the angle of the stroke (from a to b).
        double GetLength() const; ///<Returns the length of the stroke.
        vec2 GetMid() const; ///<Returns the midpoint of the stroke.
    };


    typedef std::list<Stroke> StrokeList; ///<Stores a list of strokes for input.


    class Art
    {
        public:
            typedef Spline::Hermite<vec2, double> Thread;
            typedef std::vector<Thread*> SplineVector;

            typedef Spline::Step Z;
            typedef std::vector<Z*> ZVector;


            Art(const SplineVector& sv, const ZVector& zv);
            ~Art();

            size_t GetThreadCount() const {return mThreads.size();} ///<Returns the number of separate threads in this design.

            const Thread* GetThread(size_t index) const;
            const Z* GetZ(size_t index) const;

        private:
            SplineVector mThreads;
            ZVector mZs;
    };

    typedef std::auto_ptr<Art> AutoArt;

    AutoArt CreateThread(const StrokeList& strokes); ///<Given a stroke list, creates a thread running through them. The caller should delete the splines.
}

#endif /*__CKNOT_HPP__*/
