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

#ifndef __SPLINE_HPP__
#define  __SPLINE_HPP__

#include <cassert>
#include <cmath>
#include <cstring>

///Hosts various interpolation functions.
namespace Spline
{
    ///Functions used by splines.
    namespace Function
    {
        ///The sign of a C++ % is not clearly defined.
        inline int Imod(int i, int j)
        {
            return (i % j) < 0 ? (i % j) + (j < 0 ? -j : j) : i % j;
        }

        ///Mod to loop a float around within a range.
        template <typename FT>
            FT Mod(FT i, FT start, FT end)
            {
                const FT range = end - start;
                FT d = std::fabs((i - start) / range);
                d -= std::floor(d);

                d *= range;
                if (i >= start)
                    d += start;
                else
                    d = end - d;

                assert(d >= start);
                assert(d < end);

                return d;
            }

        ///Hermite basis function.
        template <typename FT>
            FT h1(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                return 2 * t3 - 3 * t2 + 1;
            }

        template <typename FT>
            FT h2(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                return -2 * t3 + 3 * t2;
            }

        template <typename FT>
            FT h3(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                return t3 - 2 * t2 + t;
            }

        template <typename FT>
            FT h4(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                return t3 - t2;
            }

        ///Interpolates linearly between two points.
        struct Linear
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    return y0 + (y1 - y0) * t;
                }
        };

        ///A smooth polynomial acceleration.
        struct Accel
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    return Linear()(y0, y1, t * t);
                }
        };

        ///Cardinal spline, interpolates between y1 and y2 using surrounding points y0 and y3 with weight c. Assumes uniform spacing of knots.
        template <typename ST, typename FT>
            ST Cardinal(ST y0, ST y1, ST y2, ST y3, FT c, FT t)
            {
                const FT m0 = c * (y2 - y0); //Tangent for y1.
                const FT m1 = c * (y3 - y1); //Tangent for y2.
                return Hermite(m0, y1, y2, m1, t);
            }

        ///Catmull-Rom, interpolates between y1 and y2 using surrounding points y0 and y3 with weight 0.5. Assumes uniform spacing of knots.
        template <typename ST, typename FT>
            ST CatmullRom(ST y0, ST y1, ST y2, ST y3, FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;

                return ((y1 * 2) +
                        (-y0 + y2) * t +
                        (y0 * 2 - y1 * 5 + y2 * 4 - y3) * t2 +
                        (-y0 + y1 * 3 - y2 * 3 + y3) * t3) * 0.5;
            }

        ///Cosine interpolation between two points.
        struct Cosine
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    const FT pi = std::acos(-1.0);
                    return Linear()(y0, y1, -std::cos(t * pi) / 2 + 0.5);
                }
        };

        ///A smooth polynomial deceleration.
        struct Decel
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    return Linear()(y0, y1, 1 - (1 - t) * (1 - t));
                }
        };

        ///Cubic Hermite spline, taking two points and their tangents.
        /**A Cardinal splines calculates these tangents based on surrounding points and a weight parameter.
         * A Catmull-Rom spline is a cardinal spline with weight of 0.5.
         */
        template <typename ST, typename FT>
            ST Hermite(ST m0, ST y0, ST y1, ST m1, FT t)
            {
                return
                    m0 * h3(t) +
                    y0 * h1(t) +
                    y1 * h2(t) +
                    m1 * h4(t);
            }


        ///Step interpolation, jumps at t >= 1.0.
        struct LateStep
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    return t < 1.0 ? y0 : y1;
                }
        };


        ///Step interpolation, jumps at t 0.5.
        struct NearestNeighbor
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    return t < .5 ? y0 : y1;
                }
        };

        typedef NearestNeighbor Step;

        ///A smooth polynomial similar to Cosine.
        struct SmoothStep
        {
            template <typename ST, typename FT>
                ST operator()(ST y0, ST y1, FT t)
                {
                    return Linear()(y0, y1, t * t * (3 - 2 * t));
                }
        };
    }


    ///Useful antiderivatives of some functions.
    namespace Antiderivatives
    {
        template <typename FT>
            FT h1(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                const FT t4 = t3 * t;
                return t4 / 2 - t3 + t;
            }

        template <typename FT>
            FT h2(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                const FT t4 = t3 * t;
                return t3 - t4 / 2;
            }

        template <typename FT>
            FT h3(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                const FT t4 = t3 * t;
                return t4 / 4 - 2 * t3 / 3 + t2 / 2;
            }

        template <typename FT>
            FT h4(FT t)
            {
                const FT t2 = t * t;
                const FT t3 = t2 * t;
                const FT t4 = t3 * t;
                return t4 / 4 - t3 / 3;
            }
    }


    ///Useful derivatives of some functions.
    namespace Derivatives
    {
        template <typename FT>
            FT h1(FT t)
            {
                const FT t2 = t * t;
                return 6 * t2 - 6 * t;
            }

        template <typename FT>
            FT h2(FT t)
            {
                const FT t2 = t * t;
                return 6 * t - 6 * t2;
            }

        template <typename FT>
            FT h3(FT t)
            {
                const FT t2 = t * t;
                return 3 * t2 - 4 * t + 1;
            }

        template <typename FT>
            FT h4(FT t)
            {
                const FT t2 = t * t;
                return 3 * t2 - 2 * t;
            }

        template <typename ST, typename FT>
            ST Hermite(ST m0, ST y0, ST y1, ST m1, FT t)
            {
                return
                    m0 * h3(t) +
                    y0 * h1(t) +
                    y1 * h2(t) +
                    m1 * h4(t);
            }
    }


    ///Interface for interpolation between several points.
    template <typename ST, typename FT = double>
        class Spline
        {
            public:
                /**\brief ABC for spline.
                 * \param xs X values. Must always be monotonic, some splines require uniform spacing.
                 * \param ys Y value for each x.
                 * \param n Number of knots (size of x and y array).
                 * \param loop True if the spline should loop around when given at values outside of xs.
                 * In general, this should be true if values >= xs[n-1] || values < xs[0] will be given.
                 * If true, the last y value should match the first y value.
                 * \param copy If true xs and ys are copied, if false they are not (and the original data must remain valid for the life of the spline).
                 */
                Spline(const FT* xs, const ST* ys, size_t n, bool loop, bool copy)
                    :mN(n), mLoop(loop), mCopy(copy)
                {
                    assert(mN > 1);

                    if (!mCopy)
                    {
                        mXs = xs;
                        mYs = ys;
                    }
                    else
                    {
                        FT* tempXs = new FT[n];
                        std::memcpy(tempXs, xs, sizeof(FT) * n);
                        mXs = tempXs;

                        ST* tempYs = new ST[n];
                        std::memcpy(tempYs, ys, sizeof(ST) * n);
                        mYs = tempYs;
                    }

#ifndef NDEBUG
                    FT last = mXs[0];
                    for (size_t i = 1; i < mN; ++i)
                    {
                        assert(mXs[i] > last);
                        last = mXs[i];
                    }
#endif
                }

                virtual ~Spline()
                {
                    if (mCopy)
                    {
                        delete[] mXs;
                        delete[] mYs;
                    }
                }

                ST operator()(FT x) const {return Y(x);}

                virtual ST Y(FT x) const = 0;

                size_t GetKnotCount() const {return mN;}

            protected:
                const size_t mN : 30; ///<Number of data points.
                const bool mLoop : 1; ///<If true, loop outside of the x range, otherwise continue in given direction.
                const bool mCopy : 1; ///<If true, the data was copied and should not be freed here.

                ///Given an x value, returns the index before it. x can loop around.
                size_t GetIndex(FT x) const
                {
                    int i = mLastIndex;

                    //Convert x to be between mXs[0] and mXs[mN-1].
                    if (mLoop)
                        x = Function::Mod(x, mXs[0], mXs[mN-1]);

                    while (true)
                    {
                        i = Function::Imod(i, mN);

                        const FT& current = GetX(i);
                        const FT& next = GetX(i+1);

                        if (current <= x)
                        {
                            if (next > x)
                                break;
                            if (i == int(mN)-2 && !mLoop)
                                break;
                            ++i;
                        }
                        else //current > x
                        {
                            if (i == 0 && !mLoop)
                                break;
                            --i;
                        }
                    }

                    mLastIndex = i;
                    return mLastIndex;
                }

                ///Returns the amount that an x is between ranges.
                FT GetSubRange(int index, FT x) const
                {
                    //Convert x to be between mXs[0] and mXs[mN-1].
                    FT d = mLoop ? LoopInRange(x) : x;

                    //Normalize sub range.
                    const FT& sr = GetX(index);
                    const FT& er = GetX(index + 1);
                    const FT& r = er - sr;
                    d -= sr;
                    d /= r;

                    return d;
                }

                ///Returns an x value. Index will loop around.
                FT GetX(int index) const
                {
                    index = Function::Imod(index, mN);
                    assert(index >= 0);
                    assert(index < int(mN));
                    return mXs[index];
                }

                ///Returns a y value. Index will loop around.
                ST GetY(int index) const
                {
                    index = Function::Imod(index, mN);
                    assert(index >= 0);
                    assert(index < int(mN));
                    return mYs[index];
                }

                ///Loops x within the range of this spline.
                FT LoopInRange(FT x) const
                {
                    return Function::Mod(x, mXs[0], mXs[mN-1]);
                }


            private:
                mutable size_t mLastIndex; ///<Accelerates index lookup.

                const FT *mXs;
                const ST *mYs;
        };


    ///This supports non-uniform control points.
    template <typename ST = double, typename FT = double>
        class Cardinal : public Spline<ST, FT>
    {
        public:
            Cardinal(const FT* xs, const ST* ys, size_t n, bool loop, FT tension = 0.5, bool copy = true)
                :Spline<ST, FT>(xs, ys, n, loop, copy), mC(tension){}
            virtual ~Cardinal(){}

            virtual ST Y(FT x) const
            {
                const size_t i = GetIndex(x);
                const FT t = GetSubRange(i, x);

                //We interpolate between y1 and y2. y0 and y3 are needed to calculate tangents.
                //If loop is on, we should be careful to see if y0 or y3 should loop around.
                const ST y0 = i == 0 ? (this->mLoop ? this->GetY(i-2) : this->GetY(i+1)) : this->GetY(i-1);
                const ST y1 = this->GetY(i);
                const ST y2 = this->GetY(i+1);
                const ST y3 = i + 2 == this->mN ?
                    (this->mLoop ? this->GetY(i+3) : this->GetY(i)) :
                    this->GetY(i+2);

                //The x values are needed in case of non-uniform spacing.
                const FT x1 = this->GetX(i);
                const FT x2 = this->GetX(i+1);

                //Find the change in x for the x0-x1 interval (dx1) and the x2-x3 interval (dx2).
                //Looping around makes this a little bit tricky.
                const FT dx1 = (i == 0 ? (this->mLoop ? this->GetX(this->mN-1) - this->GetX(this->mN-2) : 0 ) : x1 - this->GetX(i-1));
                const FT dx2 = (i + 2 == this->mN ?
                        (this->mLoop ? this->GetX(1) - this->GetX(0) : 0 ) :
                        this->GetX(i+2) - x2);
                const FT dx = x2 - x1; //The x spacing we are going to interpolate in.

                //Tangent scaling factors.
                //(If the xs are uniform then dx = dx1 = dx2, which makes h1 = h2 = 1).
                const FT h1 = (dx / (dx1 + dx)) * 2;
                const FT h2 = (dx / (dx + dx2)) * 2;

                //Final tangents, these will be the actual slope at y1 and y2.
                //A cardinal spline sets it tangents based on the slope to surrounding points.
                const ST m1 = (y2 - y0) * h1 * mC; //y1 tangent.
                const ST m2 = (y3 - y1) * h2 * mC; //y2 tangent.

                return Function::Hermite<ST, FT>(m1, y1, y2, m2, t);
            }
        protected:
            const FT mC;
    };


    ///This only supports uniformly spaced control points. For non-uniform points use a Cardinal Spline with tension 0.5.
    template <typename ST = double, typename FT = double>
        class CatmullRom : public Spline<ST, FT>
    {
        public:
            /**If loop is false, then the first and last points use a zero tangent. So Y(xs[0]) and Y(xs[n-1]) have zero slope.
             * If more control is needed, it is recommended to add an additional point before and after the data set.
             */
            CatmullRom(const FT* xs, const ST* ys, size_t n, bool loop, bool copy = true)
                :Spline<ST, FT>(xs, ys, n, loop, copy){}
            virtual ~CatmullRom(){};
            virtual ST Y(FT x) const
            {
                const size_t i = GetIndex(x);
                const FT t = GetSubRange(i, x);
                const ST prev = i == 0 ? (this->mLoop ? this->GetY(i-2) : this->GetY(i+1)) : this->GetY(i-1);
                const ST next = i == this->mN-2 ? (this->mLoop ? this->GetY(i+3) : this->GetY(i)) : this->GetY(i+2);
                return Function::CatmullRom<ST, FT>(prev, this->GetY(i), this->GetY(i+1), next, t);
            }
        protected:
    };


    ///This creates a cubic Hermite spline and allows the user to define each tangent.
    template <typename ST = double, typename FT = double>
        class Hermite : public Spline<ST, FT>
    {
        public:
            Hermite(const FT* xs, const ST* ys, const ST* ms, size_t n, bool loop, bool copy = true)
                :Spline<ST, FT>(xs, ys, n, loop, copy)
            {
                if (!copy)
                {
                    mMs = ms;
                }
                else
                {
                    ST* tempMs = new ST[n];
                    std::memcpy(tempMs, ms, sizeof(ST) * n);
                    mMs = tempMs;
                }
            }

            virtual ~Hermite()
            {
                if (this->mCopy)
                    delete mMs;
            }

            virtual ST Y(FT x) const
            {
                const size_t i = this->GetIndex(x);
                const FT t = this->GetSubRange(i, x);

                const ST y1 = this->GetY(i);
                const ST y2 = this->GetY(i+1);

                const ST m1 = GetM(i);
                const ST m2 = GetM(i+1);

                return Function::Hermite<ST, FT>(m1, y1, y2, m2, t);
            }

        protected:
                ///Returns a m value. Index will loop around.
                ST GetM(int index) const
                {
                    index = Function::Imod(index, this->mN);
                    assert(index >= 0);
                    assert(index < int(this->mN));
                    return mMs[index];
                }

        private:
            const ST *mMs;
    };



    ///General spline that calls a function of the form f(y1, y2, t). These splines are local in that they only
    ///ever consider the two nearest points.
    template<typename T, typename ST = double, typename FT = double>
        class LocalSpline : public Spline<ST, FT>
    {
        public:
            LocalSpline(const FT* xs, const ST* ys, size_t n, bool loop, bool copy = true)
                :Spline<ST, FT>(xs, ys, n, loop, copy){}
            virtual ~LocalSpline(){};

            virtual ST Y(FT x) const
            {
                const size_t i = this->GetIndex(x);
                const FT t = this->GetSubRange(i, x);
                return T()(this->GetY(i), this->GetY(i+1), t);
            }
    };

    typedef LocalSpline<Function::Cosine> Cosine;
    typedef LocalSpline<Function::Linear> Linear;
    typedef LocalSpline<Function::LateStep> LateStep;
    typedef LocalSpline<Function::NearestNeighbor> Step;
    typedef LocalSpline<Function::SmoothStep> SmoothStep;

}

#endif
