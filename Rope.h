#ifndef ROPE_H_INCLUDED
#define ROPE_H_INCLUDED

#include <string>
#include <vector>
#include <algorithm> //for std::min
#include <iterator>

#include "RefCounter.h"
#include "RefCountedObjPtr.h"

#undef min
#undef max

#define CHUNK_SIZE 32

namespace WCRope 
{
	template< typename CharT, typename SynchronizationPrimative>
	class RopeRep : public TRefCounter<SynchronizationPrimative>
	{
		public:
			typedef RefCountedObjPtr<RopeRep> Ptr;
			typedef std::basic_string<CharT> StringType;
			
			virtual CharT Get(size_t offset)const=0;
			virtual size_t Length()const=0;			
			virtual size_t TreeDepth()const=0;

			virtual StringType GetString() const=0;

			virtual std::pair< Ptr, Ptr > GetChildren()const {
				assert(false);
				return std::pair< Ptr, Ptr >(Ptr(0),Ptr(0));
			}

			virtual ~RopeRep(){}

	};

	template< typename CharT, typename SynchronizationPrimative >
	class NullRep : public RopeRep<CharT, SynchronizationPrimative>
	{
	public:
		virtual CharT Get(size_t offset)const{
			assert(false); return 0;
		}
		virtual size_t Length()const{
			return 0;
		}
		virtual size_t TreeDepth()const{
			return 1;
		}
		virtual typename RopeRep<CharT, SynchronizationPrimative>::StringType GetString() const {
			return typename RopeRep<CharT, SynchronizationPrimative>::StringType();
		}
	};

	template< typename CharSet, typename SynchronizationPrimative >
	class StringRep : public RopeRep< CharSet, SynchronizationPrimative >
	{
		public:
            typedef typename RopeRep<CharSet, SynchronizationPrimative>::Ptr Ptr;
            typedef typename RopeRep<CharSet, SynchronizationPrimative>::StringType StringType;
                        
			StringRep( StringType const & str )
				: mStr(str)
			{                
			}

			StringRep( StringType const & lhs,  StringType const & rhs )
			{                
				mStr.reserve( lhs.size() + rhs.size() );
				mStr = lhs;
				mStr += rhs;
			}

			template< typename Itr >
			StringRep( Itr begin,  Itr end )
				: mStr( begin, end )
			{
			}

			virtual CharSet Get(size_t offset) const {
				assert(offset<mStr.length());
				return mStr[offset];
			}

			virtual size_t Length() const {
				return mStr.size();
			}

			virtual size_t TreeDepth()const {
				return 1;
			}

			virtual StringType GetString() const {
				return mStr;
			}

		private:
            StringType mStr;
	};

	template< typename CharSet, typename SynchronizationPrimative >
	class ConCatRep : public RopeRep< CharSet, SynchronizationPrimative >
	{
		public:
			typedef typename RopeRep<CharSet, SynchronizationPrimative>::Ptr Ptr;
			typedef typename RopeRep<CharSet, SynchronizationPrimative>::StringType StringType;			
                        
			ConCatRep(  Ptr const & lhs, Ptr const & rhs )
				: mLength(lhs->Length() + rhs->Length())
				, mDepth(std::max(lhs->TreeDepth(), rhs->TreeDepth())+1)
				, mLhs(lhs)
				, mRhs(rhs)
			{
			}

			~ConCatRep()
			{
				// this code flattens the callstack by "unwinding" the destruction of trees
				// prevents a stack overflow if you concatinate, ie 1000000 strings
				if (mLhs->IsUnique() || mRhs->IsUnique())
				{
					std::vector< Ptr > list;
					list.reserve(mDepth);
					
					list.push_back(mLhs);
					mLhs = 0;

					list.push_back(mRhs);
					mRhs = 0;

					size_t i=0;
					while(i!=list.size())
					{
						if (list[i]->IsUnique() && list[i]->TreeDepth()>1)
						{
							std::pair< Ptr, Ptr > p = list[i]->GetChildren();
							list.push_back(p.first);
							list[i] = p.second;
						}
						else
						{
							list[i++] = 0;
						}
					}
				}
			}

			// this code flattens the callstack by "unwinding" the traversal of the tree
			// prevents a stack overflow if you concatinate, ie 1000000 strings
			virtual CharSet Get(size_t offset) const {
				std::pair< Ptr, Ptr > p(mLhs, mRhs);
				CharSet result = 0;
				while(!result)
				{
					size_t ll = p.first->Length();
					if (offset<ll) 
					{
						if (p.first->TreeDepth()==1)
						{
							result = p.first->Get(offset);
						}
						else
						{
							p = p.first->GetChildren();
						}
					}
					else
					{
						offset -= ll;
						if (p.second->TreeDepth()==1)
						{
							result = p.second->Get(offset);
						}
						else
						{
							p = p.second->GetChildren();
						}
					}
				}

				return result;
			}

			virtual size_t Length() const {
				return mLength;
			}

			virtual size_t TreeDepth()const {
				return mDepth;
			}

			virtual std::pair< Ptr, Ptr > GetChildren()const {
				return std::pair< Ptr, Ptr >(mLhs, mRhs);
			}

			virtual StringType GetString() const {
				return mLhs->GetString() + mRhs->GetString();
			}		

		private:
			const size_t mLength;
			const size_t mDepth;
			Ptr mLhs, mRhs;
	};

	template< typename CharSet, typename SynchronizationPrimative >
	class RepeatedSequenceRep : public RopeRep< CharSet, SynchronizationPrimative >
	{
		public:
			typedef typename RopeRep<CharSet, SynchronizationPrimative>::Ptr Ptr;
			typedef typename RopeRep<CharSet, SynchronizationPrimative>::StringType StringType;
                        
			RepeatedSequenceRep(  size_t count, Ptr const & sequence )
				: mLength(count * sequence->Length())
				, mSequence(sequence)
			{
			}

			virtual CharSet Get(size_t offset) const {
				return mSequence->Get(offset % mSequence->Length());
			}

			virtual size_t Length() const {
				return mLength;
			}

			virtual size_t TreeDepth()const {
				return 1;
			}

			virtual StringType GetString() const {
				StringType result, part(mSequence->GetString());
				result.reserve(Length());
				while(result.size()!=mLength)
					result+=part;

				return result;
			}

		private:
			const size_t mLength;
			const Ptr mSequence;
	};

	template< typename CharSet, typename SynchronizationPrimative >
	class SubStrRep : public RopeRep< CharSet, SynchronizationPrimative >
	{
		public:
			typedef typename RopeRep<CharSet, SynchronizationPrimative>::Ptr Ptr;
			typedef typename RopeRep<CharSet, SynchronizationPrimative>::StringType StringType;
                        
			// half open range [start, end)
			SubStrRep(  size_t start, size_t end, Ptr const & str )
				: mStart(start)
				, mEnd(end)
				, mSequence(str)
			{
			}

			virtual CharSet Get(size_t offset) const {
				const size_t index = (mStart>mEnd) ? mStart-(offset+1) : mStart+offset;
				return mSequence->Get(index);
			}

			virtual size_t Length() const {
				return (mEnd>=mStart) ? mEnd-mStart : mStart-mEnd;
			}

			virtual size_t TreeDepth()const {
				//return mDepth;
				return 1;
			}

			virtual StringType GetString() const {
				StringType result;
				result.reserve(Length());
				for(size_t i=0;i!=Length();++i)
					result+=Get(i);

				return result;
			}

		private:
			const size_t mStart;
			const size_t mEnd;
			const Ptr mSequence;
	};

	template< typename CharT, typename SynchronizationPrimative	>
	class Rope
	{
		public:
			typedef typename RopeRep<CharT, SynchronizationPrimative>::StringType StringType;
			typedef typename RopeRep<CharT, SynchronizationPrimative>::Ptr Ptr;
            typedef CharT value_type;
			typedef const CharT* pointer;
			typedef const CharT& const_reference;
			typedef size_t size_type;

			
			// note, mutable access to chars is not permitted
			// but as with iterators, the typedef is defined as per const version
			typedef const CharT& reference; 

			// constructs a null/empty string
			Rope( )
				: mRopeRep( new NullRep<CharT, SynchronizationPrimative>() )
			{
				// nothing to do here
			}

			// constructs a copy of a string 
			Rope( const StringType& str )
			{				
				if (str.size()>0)
				{
					mRopeRep = new StringRep<CharT, SynchronizationPrimative>(str);
				}
				else
				{
					mRopeRep = new NullRep<CharT, SynchronizationPrimative>();
				}
			}

			// constructs a copy of null terminated c-string str
			// todo - needs optimising (redundant copies)
			Rope( const CharT* str )
				: mRopeRep( Rope(StringType(str)).mRopeRep )
			{

			}

			// constructs a string of "count" repetitions of rhs
			Rope( size_t count, const Rope& rhs )
				: mRopeRep( new RepeatedSequenceRep<CharT, SynchronizationPrimative>(count, rhs.mRopeRep) )
			{
				// nothing to do here
			}

			// constructs a string of "count" repetitions of char "c"
			Rope( size_t count, CharT c )
			{
				Rope result(count/CHUNK_SIZE, Rope(StringType(CHUNK_SIZE, c)) );
				result += StringType(count%CHUNK_SIZE, c);
				mRopeRep = result.mRopeRep;
			}

			// note, special case sub-str constructor for Rope::const_iterator 
			// can be found after the iterator class
			template< typename Itr >
			Rope( Itr ibegin, Itr iend )
				: mRopeRep( new StringRep<CharT, SynchronizationPrimative>(ibegin, iend) )
			{

			}

			// concatination (string)
			Rope& operator+=(const Rope& rhs)
			{
				if (rhs.size())
				{
					if (size()>0)
					{
						if (size()+rhs.size()<CHUNK_SIZE)
						{
							mRopeRep = new StringRep<CharT, SynchronizationPrimative>(
								mRopeRep->GetString(), rhs.mRopeRep->GetString()
							);
						}
						else
						{
							mRopeRep = new ConCatRep<CharT, SynchronizationPrimative>(
								mRopeRep, rhs.mRopeRep
							);
						}
					}
					else
					{
						mRopeRep = rhs.mRopeRep;
					}
				}				
				return *this;
			}

			// concatination (single character)
			// note, single character concatination does not scale well for building large strings
			Rope& operator+=(const CharT rhs)
			{
				*this += Rope( 1, rhs );
				return *this;
			}

			size_t size() const {
				return mRopeRep->Length();
			}

			size_t length() const {
				return mRopeRep->Length();
			}

			bool empty() const {
				return mRopeRep->Length() == 0;
			}

			void clear(){
				// TODO: don't allocate a null rep, have a static instance!
				mRopeRep = new NullRep<CharT, SynchronizationPrimative>();
			}

			void swap(Rope& rhs) {
				std::swap( mRopeRep, rhs.mRopeRep );
			}

			CharT front()const {
				return mRopeRep->Get(0);
			}

			CharT back()const {
				return mRopeRep->Get(size()-1);
			}

			CharT operator[](size_t n) const {
                return mRopeRep->Get(n);
			}

			// create a substring from start, of size characters in length
			Rope substr(size_t start, size_t size) const
			{
				Rope result;
				result.mRopeRep = new SubStrRep<CharT, SynchronizationPrimative>(
					start, start+size, this->mRopeRep
				);
				return result;
			}

			class const_iterator 
			{
				public:
	
					typedef typename std::forward_iterator_tag iterator_category;
					typedef CharT value_type;
					typedef size_t difference_type;
					typedef const CharT* pointer;
					typedef const CharT& reference;

					// null itr - a bit like an end to an empty string
					const_iterator()
						: mPosPtr(0)
						, mRootPtr(0)
						, mCharPos(0)
						, mIndex(0)
					{
					}

					// for constructing an end() 
					// (end itr knows how long a string it belongs too, but not what string that is!)
					explicit const_iterator(const Ptr& root, size_t end_index)
						: mPosPtr(0)
						, mRootPtr(root)
						, mCharPos(0)
						, mIndex(end_index)
					{
					}

					// a normal iterator, starts at the start of the string
					const_iterator( const Ptr& begin )
						: mPosPtr(begin)
						, mRootPtr(begin)
						, mCharPos(0)
						, mIndex(0)
					{
						if (mPosPtr.GetPtr())
						{
                            if (mPosPtr->Length() > 0)
                            {
							    mStack.reserve( mPosPtr->TreeDepth()-1 );
							    while( mPosPtr->TreeDepth()!=1 )
							    {
								    mStack.push_back( mPosPtr );
								    mPosPtr = mPosPtr->GetChildren().first;
							    }
                            }
                            else
                            {
                                // empty string, begin == end
                                mPosPtr = 0;
                            }
						}
					}

					//dereference operator, get the character at the current location
					CharT operator*() const {
						assert(mPosPtr->TreeDepth()==1);
						return mPosPtr->Get(mCharPos);
					}

					// forward stride operator, faster than random access operators for sequencial access
					const_iterator& operator+=(size_t n) 
					{
						mIndex += n;

						while(mCharPos+n>=mPosPtr->Length())
						{
							n -= mPosPtr->Length()-mCharPos;
							mCharPos = 0;
							if (!mStack.empty())
							{
								mPosPtr = mStack.back()->GetChildren().second;
								mStack.pop_back();
								while( mPosPtr->TreeDepth()!=1 )
								{
									mStack.push_back( mPosPtr );
									mPosPtr = mPosPtr->GetChildren().first;
								}
							}
							else
							{
								mPosPtr = 0;
								break;
							}
						}

						mCharPos += n;
						return *this;
					}

					// reverse stride operator, expensive 
					// - prefer random access operators on parent string for random access to char data
					// (constructs a new interator, forward strides to the right location and copys)
					const_iterator& operator-=(size_t n) 
					{
						assert( mIndex >= n );
						const_iterator result(mRootPtr);

						result += (mIndex-n);

						// swap is faster than copy, and the old copy is thrown away
						this->swap(result);

						return *this;
					}

					// pre increment, pretty fast
					const_iterator& operator++() 
					{
						*this += 1;
						return *this;
					}

					// post increment, makes a copy of the iterator, expensive on complex strings
					// to be generally avoided in favor of preincrement
					const_iterator operator++(int) 
					{
						const_iterator was(*this);
                        ++*this;
						return was;
					}

					// predecrement, expensive compared even to post increment
					// (see reverse stride operator for more info)
					const_iterator& operator--()
					{
						*this -= 1;
						return *this;
					}

					// post decrement, makes a copy of the iterator, expensive on complex strings
					// not notably more expensive than predecrement
					const_iterator operator--(int) 
					{
						assert( mIndex >= 1 );
						const_iterator result(mRootPtr);

						result += (mIndex-1);
						this->swap(result);

						return result;
					}

					// comparison, usually fast, but poor worst-case performance 
					// (worst case => iterators are the same, deep/complex tree)
					bool operator!=(const const_iterator& rhs)
					{
						if (mIndex!=rhs.mIndex ||
							mCharPos!=rhs.mCharPos ||
							mPosPtr!=rhs.mPosPtr ||
							mStack.size()!=rhs.mStack.size()) return true;

						for(size_t i=0;i!=mStack.size();++i)
							if (mStack[i]!=rhs.mStack[i]) return true;

						// should only get here if on end() of different but equal sized strings
                        return mRootPtr!=rhs.mRootPtr;
					}

					// comparison, as per != 
					bool operator==(const const_iterator& rhs)
					{
						return !(*this != rhs);
					}

					difference_type distance(const const_iterator& rhs) const {
						return rhs.mIndex - mIndex;
					}

					difference_type operator-(const const_iterator& rhs) const {
						return rhs.distance(*this);
					}

					size_t GetIndex() const {
						return mIndex;
					}

					Ptr GetRootPtr() const {
						return mRootPtr;
					}					

					void swap(const_iterator &rhs)
					{
						std::swap(mPosPtr, rhs.mPosPtr);
						std::swap(mRootPtr, rhs.mRootPtr);
						std::swap(mCharPos, rhs.mCharPos);
						std::swap(mIndex, rhs.mIndex);
						std::swap(mStack, rhs.mStack);
					}

				private:
					Ptr mPosPtr, mRootPtr;
					size_t mCharPos, mIndex;					
					typedef std::vector< Ptr > StackType;
					StackType mStack;
			};

			typedef const_iterator iterator;			

			// special case sub-str constructor
			Rope( const const_iterator& ibegin, const const_iterator& iend )
			{
                if (ibegin.distance(iend)>CHUNK_SIZE)
                {
                    mRopeRep = new SubStrRep<CharT, SynchronizationPrimative>(
                    	ibegin.GetIndex(), iend.GetIndex(), ibegin.GetRootPtr()
                    );
                }
                else 
                {
                    mRopeRep = new StringRep<CharT, SynchronizationPrimative>(
                    	ibegin, iend
                    );
                }

			}

			const_iterator begin() const {
                return const_iterator(mRopeRep);
			}

			const_iterator end() const {
				//end is null ptr
				return const_iterator( mRopeRep, size() );
			}

			//returns -1 if this < rhs, 1 if this > rhs, and 0 if this == rhs
			int LexicographicalCompare3Way(const Rope& rhs)const
			{
				std::vector< RopeRep<CharT, SynchronizationPrimative>* > lhsStack, rhsStack;
				size_t lhsCharPos(0),rhsCharPos(0);
				RopeRep<CharT, SynchronizationPrimative>* lhsPosPtr(mRopeRep.GetPtr());
				RopeRep<CharT, SynchronizationPrimative>* rhsPosPtr(rhs.mRopeRep.GetPtr());

				lhsStack.reserve( lhsPosPtr->TreeDepth()-1 );
				rhsStack.reserve( rhsPosPtr->TreeDepth()-1 );

				//const size_t rhsLength = rhs.size();
				//const size_t minLength = std::min(size(),rhsLength);
				size_t absCharPos = 0;
				
				while(lhsPosPtr && rhsPosPtr)
				{
					//travel down both trees, trying to identify shared sub trees
					if (lhsPosPtr!=rhsPosPtr)
					{
						if (lhsPosPtr->TreeDepth()==1 && rhsPosPtr->TreeDepth()==1)
						{
							while(lhsCharPos!=lhsPosPtr->Length() && rhsCharPos!=rhsPosPtr->Length()) {
								CharT l = lhsPosPtr->Get(lhsCharPos++);
								CharT r = rhsPosPtr->Get(rhsCharPos++);
								++absCharPos;
								if (l<r) return -1;
								if (r<l) return 1;
							} 

							if (lhsCharPos==lhsPosPtr->Length())
							{
								lhsCharPos = 0;
								if (lhsStack.empty())
								{
									lhsPosPtr = 0;
								}
								else
								{
									lhsPosPtr = lhsStack.back()->GetChildren().second.GetPtr();
								}								
							}

							if (rhsCharPos==rhsPosPtr->Length())
							{
								rhsCharPos = 0;
								if (rhsStack.empty())
								{
									rhsPosPtr = 0;
								}
								else
								{
									rhsPosPtr = rhsStack.back()->GetChildren().second.GetPtr();
								}								
							}
						}
						else 
						{
							//left hand sub tree fragment is larger than right hand sub tree fragment
							if ( lhsPosPtr->TreeDepth()>1 && 
								 ((lhsPosPtr->Length()-lhsCharPos > rhsPosPtr->Length()-rhsCharPos) || rhsPosPtr->TreeDepth() == 1)
								)
							{
								//dig down into left hand sub tree							
								assert(lhsPosPtr->TreeDepth()>1);
								lhsStack.push_back( lhsPosPtr );

								lhsCharPos = 0;
								lhsPosPtr = lhsPosPtr->GetChildren().first.GetPtr();
							}
							//right hand sub tree it larger than left hand sub tree
							//or sub trees are of equal size
							else
							{
								assert(rhsPosPtr->TreeDepth()>1);
								rhsStack.push_back( rhsPosPtr );

								rhsCharPos = 0;
								rhsPosPtr = rhsPosPtr->GetChildren().first.GetPtr();
							}
						}
					}
					else
					{
						//same sub tree, can skip as are equal
						assert(lhsPosPtr->Length()==rhsPosPtr->Length());
						absCharPos += lhsPosPtr->Length();
						
						lhsCharPos = 0;
						if (lhsStack.empty())
						{
							lhsPosPtr = 0;
						}
						else
						{
							lhsPosPtr = lhsStack.back()->GetChildren().second.GetPtr();
							lhsStack.pop_back();
						}

						rhsCharPos = 0;
						if (rhsStack.empty())
						{
							rhsPosPtr = 0;
						}
						else
						{
							rhsPosPtr = rhsStack.back()->GetChildren().second.GetPtr();
							rhsStack.pop_back();						
						}
					}					
				}

				//reached end of lhs string
				if (!lhsPosPtr)
				{
					//reached end of rhs string
					if (!rhsPosPtr)
					{
						//strings are equal
						return 0;
					}
					else
					{
						//strings are equal up to end of lhs
						//but rhs is longer, so lhs < rhs
						return -1; 
					}
				}
				else
				{
					//should have reached end of rhs string
					assert(!rhsPosPtr);

					//strings are equal up to end of rhs
					//but lhs is longer, so rhs < lhs
					return 1; 
				}
			}

			bool operator<(const Rope& rhs)const{
				return LexicographicalCompare3Way(rhs) < 0;
			}

			bool operator==(const Rope& rhs)const{
				return LexicographicalCompare3Way(rhs) == 0;
			}

			bool operator!=(const Rope& rhs)const{
				return LexicographicalCompare3Way(rhs) != 0;
			}

			bool operator==(const StringType& rhs)const{
				if (rhs.size()!=size()) return false;
				const_iterator e = end();
				const_iterator ri = begin();
				typename StringType::const_iterator si = rhs.begin();
				for(;ri!=e;++ri,++si)
					if (*si!=*ri)
						return false;

				return true;
			}

			bool operator==(const CharT* rhs)const{
				const_iterator e = end();
				const_iterator ri = begin();
				for(;ri!=e;++ri,++rhs)
				{
					if (*rhs!=*ri || *rhs==0)
						return false;
				}

				return *rhs==0;
			}
            
            const_iterator find_next(const CharT rhs, const_iterator ri) const
            {
                const_iterator e = end();
                for(;ri!=e;++ri)
                    if (*ri==rhs)
                        break;
                return ri;
            }
        
            const_iterator find(const CharT rhs) const
            {
                return find_next(rhs, begin());
            }
                    
            const_iterator find(const CharT* rhs) const
            {
                const_iterator ri = begin();
                const_iterator e = end();
                for(;ri!=e;++ri)
				{
                    if (*rhs==*ri)
                    {
                        const CharT* rhs_pos = rhs;
                        const_iterator pos = ri;
                        do {
                            rhs_pos ++;
                            pos ++;
                        } while (*rhs_pos && pos!=e && *rhs_pos==*pos);
                        if (*rhs_pos==0) 
                            return ri;
					}
				}
                return e;
            }

			// warning, may be expensive
			StringType GetString() const {
				return mRopeRep->GetString();
			}

			template< typename scalar >
			scalar AsDecimal()const
			{
				scalar result = 0;
			 
				if (!empty())
				{
					const_iterator i = this->begin();
					
					const bool negate = (*i=='-');
					if (negate)
						++i;

					for(;i!=this->end() && isdigit(*i);++i)
					{
						result *= 10;
						result += *i - '0';
					}

					if (negate)
						result = -result;
				}
			    
				return result;
			}


		protected:
			Ptr mRopeRep;
	};

	template< typename CharT, typename SynchronizationPrimative >
	Rope<CharT, SynchronizationPrimative> operator+(
		const Rope<CharT, SynchronizationPrimative>& lhs, 
		const Rope<CharT, SynchronizationPrimative>& rhs)
	{
		Rope<CharT, SynchronizationPrimative> result(lhs);
		result += rhs;
		return result;
	}

	template< typename CharT, typename SynchronizationPrimative >
	Rope<CharT, SynchronizationPrimative> operator+(
		const Rope<CharT, SynchronizationPrimative>& lhs, 
		const CharT* rhs)
	{
		Rope<CharT, SynchronizationPrimative> result(lhs);
		result += Rope<CharT, SynchronizationPrimative>(rhs);
		return result;
	}

	template< typename CharT, typename SynchronizationPrimative >
	bool operator==(
		const typename Rope<CharT, SynchronizationPrimative>::StringType& lhs, 
		const Rope<CharT, SynchronizationPrimative>& rhs)
	{
		return rhs==lhs;
	}

	template< typename CharT, typename SynchronizationPrimative >
	bool operator==(
		const CharT* lhs, 
		const Rope<CharT, SynchronizationPrimative>& rhs)
	{
		return rhs==lhs;
	}

	template<typename char_t, typename SynchronizationPrimative>
	std::ostream& operator<<(
		std::ostream& os, 
		const WCRope::Rope<char_t, SynchronizationPrimative>& rhs)
	{
		typedef typename WCRope::Rope<char_t, SynchronizationPrimative>::const_iterator itr;
		for (itr i = rhs.begin(); i!=rhs.end(); ++i)
		{
			os << *i;
		}
		return os;
	}

	template< typename CharT, typename SynchronizationPrimative >
	class ReversableRope : public Rope<CharT, SynchronizationPrimative>
	{
		public:
			typedef Rope<CharT, SynchronizationPrimative> Base;
			// constructs a null/empty string
			ReversableRope( )
				: Base( )
			{
				// nothing to do here
			}

			// constructs a copy of a string 
			ReversableRope( const typename Rope<CharT, SynchronizationPrimative>::StringType& str )
				: Base(str)
			{				

			}

			// constructs a copy of null terminated c-string str
			// todo - needs optimising (redundant copies)
			ReversableRope( const CharT* str )
				: Base(str)
			{
				// nothing to do here
			}

			// constructs a string of "count" repetitions of rhs
			ReversableRope( size_t count, Rope<CharT, SynchronizationPrimative>& rhs )
				: Base( count, rhs )
			{
				// nothing to do here
			}

			// constructs a string of "count" repetitions of char "c"
			ReversableRope( size_t count, CharT c )
				: Base( count, c )
			{
				// nothing to do here			
			}

			template< typename Itr >
			ReversableRope( Itr ibegin, Itr iend )
				: Base( ibegin, iend )
			{
				// nothing to do here
			}

			ReversableRope(const Rope<CharT, SynchronizationPrimative>& rhs)
				: Base(rhs)
			{
				// nothing to do here
			}

			// create a reversed representation of this string
			// a ref to the rev string is kept, and the reverse's reverse is referenced back to this, 
			// so that repeated reverse ops don't create reduant objects, 
			// and (reverse().reverse()==*this) is fast
			ReversableRope reverse() const
			{				
				if (mRevRep.GetPtr()==0)
				{
					mRevRep = new SubStrRep<CharT, SynchronizationPrimative>(
						this->size(), 0, this->mRopeRep
					);
				}
				ReversableRope result;
				result.mRopeRep = mRevRep;
				result.mRevRep = this->mRopeRep;
				return result;
			}

			// reverse iteration is a bit of a hack...
			typedef typename Rope<CharT, SynchronizationPrimative>::const_iterator const_reverse_iterator;
			typedef typename Rope<CharT, SynchronizationPrimative>::const_iterator reverse_iterator;

			const_reverse_iterator rbegin() const {
				// creates a new rope that is the reverse of this one, then returns the begin() of that
                return reverse().begin();
			}

			const_reverse_iterator rend() const {
				//works because all rope 'end' iterators are basically the same (when length is same)
                return reverse().end();
			}

		private:
			mutable typename Rope<CharT, SynchronizationPrimative>::Ptr mRevRep;
	};
}

#endif
