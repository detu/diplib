/*
 * DIPlib 3.0
 * This file contains definitions for the Image class and related functions.
 *
 * (c)2014-2015, Cris Luengo.
 * Based on original DIPlib code: (c)1995-2014, Delft University of Technology.
 */

#include "diplib.h"

using namespace dip;


Image Image::operator[]( const UnsignedArray& indices ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   uint i = 0;
   uint j = 0;
   switch (indices.size()) {
      case 2:
         j = indices[1];
      case 1:
         i = indices[0];
         break;
      default:
         Throw( E::ARRAY_ILLEGAL_SIZE );
   }
   uint m = tensor.Rows();
   uint n = tensor.Columns();
   ThrowIf( ( i >= m ) || ( j >= n ), E::INDEX_OUT_OF_RANGE );
   switch( tensor.GetShape() ) {
      case Tensor::Shape::COL_VECTOR:
         break;
      case Tensor::Shape::ROW_VECTOR:
         i = j;
         break;
      case Tensor::Shape::ROW_MAJOR_MATRIX:
         std::swap( i, j );
      case Tensor::Shape::COL_MAJOR_MATRIX:
         i += j*m;
         break;
      case Tensor::Shape::DIAGONAL_MATRIX:
         ThrowIf( i != j, E::INDEX_OUT_OF_RANGE );
         break;
      case Tensor::Shape::LOWTRIANG_MATRIX:
         std::swap( i, j );
      case Tensor::Shape::UPPTRIANG_MATRIX:
         ThrowIf( i > j , E::INDEX_OUT_OF_RANGE );
         // |0 4 5 6|     |0 1 2|
         // |x 1 7 8| --\ |x 3 4| (index + 4)
         // |x x 2 9| --/ |x x 5|
         // |x x x 3|
      case Tensor::Shape::SYMMETRIC_MATRIX:
         if( i != j ) {
            if( i > j ) std::swap( i, j );
            // we know: j >= 1
            uint k = 0;
            for( uint ii = 0; ii<i; ++ii ) {
               --j;
               --n;
               k += n;
            }
            --j;
            k += j;
            i = k + m;
         }
         break;
   }
   // Now `i` contains the linear index to the tensor element.
   Image out = *this;
   out.tensor.SetScalar();
   out.origin = (uint8*)origin + i * tstride * datatype.SizeOf();
   return out;
}

Image Image::operator[]( uint index ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( index >= tensor.Elements() , E::INDEX_OUT_OF_RANGE );
   Image out = *this;
   out.tensor.SetScalar();
   out.origin = (uint8*)origin + index * tstride * datatype.SizeOf();
   return out;
}

Image Image::Diagonal() const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   if( tensor.IsScalar() || tensor.IsDiagonal() ) {
      return *this;
   } else if( tensor.IsVector() ) {
      Image out = *this;
      out.tensor.SetScalar();                // Keep the first tensor element only
      return out;
   } else if( tensor.IsSymmetric() || tensor.IsTriangular() ) {
      Image out = *this;
      out.tensor.SetVector(tensor.Rows());   // The diagonal elements are the first ones.
      return out;
   } else { // matrix
      Image out = *this;
      uint m = tensor.Rows();
      uint n = tensor.Columns();
      out.tensor.SetVector(std::min(m,n));
      if (tensor.GetShape() == Tensor::Shape::COL_MAJOR_MATRIX) {
         out.tstride = (m+1)*tstride;
      } else { // row-major matrix
         out.tstride = (n+1)*tstride;
      }
      return out;
   }
}

Image Image::At( const UnsignedArray& coords ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( coords.size() != dims.size(), E::ARRAY_ILLEGAL_SIZE );
   uint index = 0;
   for( uint ii=0; ii<coords.size(); ++ii ) {
      ThrowIf( coords[ii] >= dims[ii], E::INDEX_OUT_OF_RANGE );
      index += coords[ii]*strides[ii];
   }
   Image out = *this;
   out.dims.resize( 0 );
   out.strides.resize( 0 );
   out.origin = (uint8*)origin + index * datatype.SizeOf();
   return out;
}

Image Image::At( uint index ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   if( dims.size() < 2 )
   {
      uint n = dims.size()==0 ? 1 : dims[0];
      ThrowIf( index >= n, E::INDEX_OUT_OF_RANGE );
      Image out = *this;
      out.dims.resize( 0 );
      out.strides.resize( 0 );
      out.origin = (uint8*)origin + index * datatype.SizeOf();
      return out;
   }
   else
   {
      return At( IndexToCoordinate( index ) );
   }
}

Image Image::At( Range x_range ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( dims.size() != 1, E::ILLEGAL_DIMENSIONALITY );
   x_range.Fix( dims[0] );
   Image out = *this;
   out.dims[0] = x_range.Size();
   out.strides[0] *= x_range.Step();
   out.origin = (uint8*)origin + x_range.Offset() * strides[0] * datatype.SizeOf();
   return out;
}

Image Image::At( Range x_range, Range y_range ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( dims.size() != 2, E::ILLEGAL_DIMENSIONALITY );
   x_range.Fix( dims[0] );
   y_range.Fix( dims[1] );
   Image out = *this;
   out.dims[0] = x_range.Size();
   out.dims[1] = y_range.Size();
   out.strides[0] *= x_range.Step();
   out.strides[1] *= y_range.Step();
   out.origin = (uint8*)origin + ( x_range.Offset() * strides[0] +
                                   y_range.Offset() * strides[1] ) * datatype.SizeOf();
   return out;
}

Image Image::At( Range x_range, Range y_range, Range z_range ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( dims.size() != 3, E::ILLEGAL_DIMENSIONALITY );
   x_range.Fix( dims[0] );
   y_range.Fix( dims[1] );
   z_range.Fix( dims[2] );
   Image out = *this;
   out.dims[0] = x_range.Size();
   out.dims[1] = y_range.Size();
   out.dims[2] = z_range.Size();
   out.strides[0] *= x_range.Step();
   out.strides[1] *= y_range.Step();
   out.strides[2] *= z_range.Step();
   out.origin = (uint8*)origin + ( x_range.Offset() * strides[0] +
                                   y_range.Offset() * strides[1] +
                                   z_range.Offset() * strides[2] ) * datatype.SizeOf();
   return out;
}

Image Image::At( RangeArray ranges ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( dims.size() != ranges.size(), E::ARRAY_ILLEGAL_SIZE );
   for( uint ii = 0; ii < dims.size(); ++ii ) {
      ranges[ii].Fix( dims[ii] );
   }
   Image out = *this;
   uint index = 0;
   for( uint ii = 0; ii < dims.size(); ++ii ) {
      out.strides[ii] *= ranges[ii].Step();
      out.dims[ii] = ranges[ii].Size();
      index += ranges[ii].Offset() * strides[ii];
   }
   out.origin = (uint8*)origin + index * datatype.SizeOf();
   return out;
}

void dip::DefineROI(
   const Image& src,
   Image& dest,
   const UnsignedArray& origin,
   const UnsignedArray& dims,
   const IntegerArray& spacing
){
   ThrowIf( !dest.IsForged(), E::IMAGE_NOT_FORGED );
   uint n = src.Dimensionality();
   ThrowIf( origin.size()  != n , E::ARRAY_ILLEGAL_SIZE );
   ThrowIf( dims.size()    != n , E::ARRAY_ILLEGAL_SIZE );
   ThrowIf( spacing.size() != n , E::ARRAY_ILLEGAL_SIZE );
   RangeArray ranges( n );
   for( uint ii=0; ii<n; ++ii ) {
      ranges[ii] = Range( origin[ii], dims[ii]+origin[ii]-1, spacing[ii] );
   }
   dest = src.At( ranges );
}

dip::uint Image::CoordinateToIndex( UnsignedArray& coords ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   ThrowIf( coords.size() != dims.size(), E::ARRAY_ILLEGAL_SIZE );
   uint index = 0;
   uint stride = 1;
   for( uint ii=0; ii<coords.size(); ++ii ) {
      ThrowIf( coords[ii] >= dims[ii], E::INDEX_OUT_OF_RANGE );
      index += coords[ii]*stride;
      stride *= dims[ii];
   }
   return index;
}

UnsignedArray Image::IndexToCoordinate( uint index ) const {
   ThrowIf( !IsForged(), E::IMAGE_NOT_FORGED );
   UnsignedArray coords( dims.size() );
   UnsignedArray fake_strides( dims.size() );
   uint stride = 1;
   for( uint ii=0; ii<dims.size(); ++ii ) {
      fake_strides[ii] = stride;
      stride *= dims[ii];
   }
   for( sint ii=((sint)dims.size())-1; ii>=0; --ii ) {
      coords[ii] = index / strides[ii];
      index      = index % strides[ii];
      ThrowIf( coords[ii] >= dims[ii], E::INDEX_OUT_OF_RANGE );
   }
   return coords;
}