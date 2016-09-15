/*
    Copyright (c) 2005-2016 Intel Corporation

    Licensed under the Apache License, Version 2.0 (the "License");
    you may not use this file except in compliance with the License.
    You may obtain a copy of the License at

        http://www.apache.org/licenses/LICENSE-2.0

    Unless required by applicable law or agreed to in writing, software
    distributed under the License is distributed on an "AS IS" BASIS,
    WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
    See the License for the specific language governing permissions and
    limitations under the License.




*/

#include "tbb/blocked_range3d.h"
#include "harness_assert.h"

// First test as much as we can without including other headers.
// Doing so should catch problems arising from failing to include headers.

template<typename Tag>
class AbstractValueType {
    AbstractValueType() {}
    int value;
public:
    template<typename OtherTag>
    friend AbstractValueType<OtherTag> MakeAbstractValueType( int i );

    template<typename OtherTag>
    friend int GetValueOf( const AbstractValueType<OtherTag>& v ) ;
};

template<typename Tag>
AbstractValueType<Tag> MakeAbstractValueType( int i ) {
    AbstractValueType<Tag> x;
    x.value = i;
    return x;
}

template<typename Tag>
int GetValueOf( const AbstractValueType<Tag>& v ) {return v.value;}

template<typename Tag>
bool operator<( const AbstractValueType<Tag>& u, const AbstractValueType<Tag>& v ) {
    return GetValueOf(u)<GetValueOf(v);
}

template<typename Tag>
std::size_t operator-( const AbstractValueType<Tag>& u, const AbstractValueType<Tag>& v ) {
    return GetValueOf(u)-GetValueOf(v);
}

template<typename Tag>
AbstractValueType<Tag> operator+( const AbstractValueType<Tag>& u, std::size_t offset ) {
    return MakeAbstractValueType<Tag>(GetValueOf(u)+int(offset));
}

struct PageTag {};
struct RowTag {};
struct ColTag {};

static void SerialTest() {
    typedef AbstractValueType<PageTag> page_type;
    typedef AbstractValueType<RowTag> row_type;
    typedef AbstractValueType<ColTag> col_type;
    typedef tbb::blocked_range3d<page_type,row_type,col_type> range_type;
    for( int pagex=-4; pagex<4; ++pagex ) {
        for( int pagey=pagex; pagey<4; ++pagey ) {
            page_type pagei = MakeAbstractValueType<PageTag>(pagex);
            page_type pagej = MakeAbstractValueType<PageTag>(pagey);
            for( int pageg=1; pageg<4; ++pageg ) {
                for( int rowx=-4; rowx<4; ++rowx ) {
                    for( int rowy=rowx; rowy<4; ++rowy ) {
                        row_type rowi = MakeAbstractValueType<RowTag>(rowx);
                        row_type rowj = MakeAbstractValueType<RowTag>(rowy);
                        for( int rowg=1; rowg<4; ++rowg ) {
                            for( int colx=-4; colx<4; ++colx ) {
                                for( int coly=colx; coly<4; ++coly ) {
                                    col_type coli = MakeAbstractValueType<ColTag>(colx);
                                    col_type colj = MakeAbstractValueType<ColTag>(coly);
                                    for( int colg=1; colg<4; ++colg ) {
                                        range_type r( pagei, pagej, pageg, rowi, rowj, rowg, coli, colj, colg );
                                        AssertSameType( r.is_divisible(), true );

                                        AssertSameType( r.empty(), true );

                                        AssertSameType( static_cast<range_type::page_range_type::const_iterator*>(0), static_cast<page_type*>(0) );
                                        AssertSameType( static_cast<range_type::row_range_type::const_iterator*>(0), static_cast<row_type*>(0) );
                                        AssertSameType( static_cast<range_type::col_range_type::const_iterator*>(0), static_cast<col_type*>(0) );

                                        AssertSameType( r.pages(), tbb::blocked_range<page_type>( pagei, pagej, 1 ));
                                        AssertSameType( r.rows(), tbb::blocked_range<row_type>( rowi, rowj, 1 ));
                                        AssertSameType( r.cols(), tbb::blocked_range<col_type>( coli, colj, 1 ));

                                        ASSERT( r.empty()==(pagex==pagey||rowx==rowy||colx==coly), NULL );

                                        ASSERT( r.is_divisible()==(pagey-pagex>pageg||rowy-rowx>rowg||coly-colx>colg), NULL );

                                        if( r.is_divisible() ) {
                                            range_type r2(r,tbb::split());
                                            if( (GetValueOf(r2.pages().begin())==GetValueOf(r.pages().begin())) && (GetValueOf(r2.rows().begin())==GetValueOf(r.rows().begin())) ) {
                                                ASSERT( GetValueOf(r2.pages().end())==GetValueOf(r.pages().end()), NULL );
                                                ASSERT( GetValueOf(r2.rows().end())==GetValueOf(r.rows().end()), NULL );
                                                ASSERT( GetValueOf(r2.cols().begin())==GetValueOf(r.cols().end()), NULL );
                                            } else {
                                                if ( (GetValueOf(r2.pages().begin())==GetValueOf(r.pages().begin())) && (GetValueOf(r2.cols().begin())==GetValueOf(r.cols().begin())) ) {
                                                    ASSERT( GetValueOf(r2.pages().end())==GetValueOf(r.pages().end()), NULL );
                                                    ASSERT( GetValueOf(r2.cols().end())==GetValueOf(r.cols().end()), NULL );
                                                    ASSERT( GetValueOf(r2.rows().begin())==GetValueOf(r.rows().end()), NULL );
                                                } else {
                                                   ASSERT( GetValueOf(r2.rows().end())==GetValueOf(r.rows().end()), NULL );
                                                   ASSERT( GetValueOf(r2.cols().end())==GetValueOf(r.cols().end()), NULL );
                                                   ASSERT( GetValueOf(r2.pages().begin())==GetValueOf(r.pages().end()), NULL );
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }
}

#include "tbb/parallel_for.h"
#include "harness.h"

const int N = 1<<5;

unsigned char Array[N][N][N];

struct Striker {
   // Note: we use <int> here instead of <long> in order to test for problems similar to Quad 407676
    void operator()( const tbb::blocked_range3d<int>& r ) const {
        for( tbb::blocked_range<int>::const_iterator i=r.pages().begin(); i!=r.pages().end(); ++i )
            for( tbb::blocked_range<int>::const_iterator j=r.rows().begin(); j!=r.rows().end(); ++j )
                for( tbb::blocked_range<int>::const_iterator k=r.cols().begin(); k!=r.cols().end(); ++k )
                    ++Array[i][j][k];
    }
};

void ParallelTest() {
    for( int i=0; i<N; i=i<3 ? i+1 : i*3 ) {
        for( int j=0; j<N; j=j<3 ? j+1 : j*3 ) {
            for( int k=0; k<N; k=k<3 ? k+1 : k*3 ) {
                const tbb::blocked_range3d<int> r( 0, i, 5, 0, j, 3, 0, k, 1 );
                tbb::parallel_for( r, Striker() );
                for( int l=0; l<N; ++l ) {
                    for( int m=0; m<N; ++m ) {
                        for( int n=0; n<N; ++n ) {
                             ASSERT( Array[l][m][n]==(l<i && m<j && n<k), NULL );
                             Array[l][m][n] = 0;
                        }
                    }
                }
            }
        }
    }
}

#include "tbb/task_scheduler_init.h"

int TestMain () {
    SerialTest();
    for( int p=MinThread; p<=MaxThread; ++p ) {
        tbb::task_scheduler_init init(p);
        ParallelTest();
    }
    return Harness::Done;
}
