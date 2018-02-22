dnl Process this m4 file to produce 'C' language file.
dnl
dnl If you see this line, you can ignore the next one.
! Do not edit this file. It is produced from the corresponding .m4 source
dnl
!
!  Copyright (C) 2014, Northwestern University and Argonne National Laboratory
!  See COPYRIGHT notice in top-level directory.
!
! $Id: getput_varn.m4 2221 2015-12-12 00:39:15Z wkliao $
!

dnl
dnl VARN1
dnl
define(`VARN1',dnl
`dnl
   ! $1 a scalar of type $5 (kind=$4)
   function nf90mpi_$1_varn_$4$2(ncid, varid, value, start)
     integer,                                        intent(in) :: ncid, varid
     $5 (kind=$4),                                   intent($3) :: value
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), intent(in) :: start
     integer                                                    :: nf90mpi_$1_varn_$4$2
 
     nf90mpi_$1_varn_$4$2 = nfmpi_$1_var1_$6$2(ncid, varid, start(:,1), value)
   end function nf90mpi_$1_varn_$4$2
')dnl

VARN1(put,     , in,    OneByteInt,    integer, int1)
VARN1(put,     , INTENTV, TwoByteInt,    integer, int2)
VARN1(put,     , INTENTV, FourByteInt,   integer, int)
VARN1(put,     , INTENTV, FourByteReal,  real,    real)
VARN1(put,     , INTENTV, EightByteReal, real,    double)
VARN1(put,     , INTENTV, EightByteInt,  integer, int8)

VARN1(put, _all, in,    OneByteInt,    integer, int1)
VARN1(put, _all, INTENTV, TwoByteInt,    integer, int2)
VARN1(put, _all, INTENTV, FourByteInt,   integer, int)
VARN1(put, _all, INTENTV, FourByteReal,  real,    real)
VARN1(put, _all, INTENTV, EightByteReal, real,    double)
VARN1(put, _all, INTENTV, EightByteInt,  integer, int8)

VARN1(get,     , out,   OneByteInt,    integer, int1)
VARN1(get,     , out,   TwoByteInt,    integer, int2)
VARN1(get,     , out,   FourByteInt,   integer, int)
VARN1(get,     , out,   FourByteReal,  real,    real)
VARN1(get,     , out,   EightByteReal, real,    double)
VARN1(get,     , out,   EightByteInt,  integer, int8)

VARN1(get, _all, out,   OneByteInt,    integer, int1)
VARN1(get, _all, out,   TwoByteInt,    integer, int2)
VARN1(get, _all, out,   FourByteInt,   integer, int)
VARN1(get, _all, out,   FourByteReal,  real,    real)
VARN1(get, _all, out,   EightByteReal, real,    double)
VARN1(get, _all, out,   EightByteInt,  integer, int8)

dnl
dnl VARN(ncid, varid, values, num, start, count)
dnl
define(`VARN',dnl
`dnl
   function nf90mpi_$1_varn_$2_$3$8(ncid, varid, values, num, start, count)
     integer,                                                  intent(in) :: ncid, varid, num
     $4 (kind=$3),                   dimension($6),            intent($7) :: values
     integer (kind=MPI_OFFSET_KIND), dimension(:,:),           intent(in) :: start
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), optional, intent(in) :: count
     integer                                                              :: nf90mpi_$1_varn_$2_$3$8
     integer (kind=MPI_OFFSET_KIND), dimension(nf90_max_var_dims,num)     :: localCount
     integer                                                              :: numDims
 
     ! Set local arguments to default values
     numDims = size(start(:,1))
     localCount(1:numDims,1:num) = 1
     if (present(count)) localCount(1:numDims,1:num) = count(1:numDims,1:num)
     nf90mpi_$1_varn_$2_$3$8 = nfmpi_$1_varn_$5$8(ncid, varid, num, start, &
                                                  localCount(1:numDims,1:num), values)
   end function nf90mpi_$1_varn_$2_$3$8
')dnl

!
! put APIs
!

VARN(put, 1D, OneByteInt, integer, int1,  :,              in)
VARN(put, 2D, OneByteInt, integer, int1, `:,:',           in)
VARN(put, 3D, OneByteInt, integer, int1, `:,:,:',         in)
VARN(put, 4D, OneByteInt, integer, int1, `:,:,:,:',       in)
VARN(put, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     in)
VARN(put, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   in)
VARN(put, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', in)

VARN(put, 1D, TwoByteInt, integer, int2,  :,              INTENTV)
VARN(put, 2D, TwoByteInt, integer, int2, `:,:',           INTENTV)
VARN(put, 3D, TwoByteInt, integer, int2, `:,:,:',         INTENTV)
VARN(put, 4D, TwoByteInt, integer, int2, `:,:,:,:',       INTENTV)
VARN(put, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     INTENTV)
VARN(put, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   INTENTV)
VARN(put, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', INTENTV)

VARN(put, 1D, FourByteInt, integer, int,  :,              INTENTV)
VARN(put, 2D, FourByteInt, integer, int, `:,:',           INTENTV)
VARN(put, 3D, FourByteInt, integer, int, `:,:,:',         INTENTV)
VARN(put, 4D, FourByteInt, integer, int, `:,:,:,:',       INTENTV)
VARN(put, 5D, FourByteInt, integer, int, `:,:,:,:,:',     INTENTV)
VARN(put, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   INTENTV)
VARN(put, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', INTENTV)

VARN(put, 1D, FourByteReal, real,   real,  :,              INTENTV)
VARN(put, 2D, FourByteReal, real,   real, `:,:',           INTENTV)
VARN(put, 3D, FourByteReal, real,   real, `:,:,:',         INTENTV)
VARN(put, 4D, FourByteReal, real,   real, `:,:,:,:',       INTENTV)
VARN(put, 5D, FourByteReal, real,   real, `:,:,:,:,:',     INTENTV)
VARN(put, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   INTENTV)
VARN(put, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', INTENTV)

VARN(put, 1D, EightByteReal, real, double,  :,              INTENTV)
VARN(put, 2D, EightByteReal, real, double, `:,:',           INTENTV)
VARN(put, 3D, EightByteReal, real, double, `:,:,:',         INTENTV)
VARN(put, 4D, EightByteReal, real, double, `:,:,:,:',       INTENTV)
VARN(put, 5D, EightByteReal, real, double, `:,:,:,:,:',     INTENTV)
VARN(put, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   INTENTV)
VARN(put, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', INTENTV)

VARN(put, 1D, EightByteInt, integer, int8,  :,              INTENTV)
VARN(put, 2D, EightByteInt, integer, int8, `:,:',           INTENTV)
VARN(put, 3D, EightByteInt, integer, int8, `:,:,:',         INTENTV)
VARN(put, 4D, EightByteInt, integer, int8, `:,:,:,:',       INTENTV)
VARN(put, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     INTENTV)
VARN(put, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   INTENTV)
VARN(put, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', INTENTV)

!
! get APIs
!

VARN(get, 1D, OneByteInt, integer, int1,  :,              out)
VARN(get, 2D, OneByteInt, integer, int1, `:,:',           out)
VARN(get, 3D, OneByteInt, integer, int1, `:,:,:',         out)
VARN(get, 4D, OneByteInt, integer, int1, `:,:,:,:',       out)
VARN(get, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     out)
VARN(get, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   out)
VARN(get, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', out)

VARN(get, 1D, TwoByteInt, integer, int2,  :,              out)
VARN(get, 2D, TwoByteInt, integer, int2, `:,:',           out)
VARN(get, 3D, TwoByteInt, integer, int2, `:,:,:',         out)
VARN(get, 4D, TwoByteInt, integer, int2, `:,:,:,:',       out)
VARN(get, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     out)
VARN(get, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   out)
VARN(get, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', out)

VARN(get, 1D, FourByteInt, integer, int,  :,              out)
VARN(get, 2D, FourByteInt, integer, int, `:,:',           out)
VARN(get, 3D, FourByteInt, integer, int, `:,:,:',         out)
VARN(get, 4D, FourByteInt, integer, int, `:,:,:,:',       out)
VARN(get, 5D, FourByteInt, integer, int, `:,:,:,:,:',     out)
VARN(get, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   out)
VARN(get, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', out)

VARN(get, 1D, FourByteReal, real,   real,  :,              out)
VARN(get, 2D, FourByteReal, real,   real, `:,:',           out)
VARN(get, 3D, FourByteReal, real,   real, `:,:,:',         out)
VARN(get, 4D, FourByteReal, real,   real, `:,:,:,:',       out)
VARN(get, 5D, FourByteReal, real,   real, `:,:,:,:,:',     out)
VARN(get, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   out)
VARN(get, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', out)

VARN(get, 1D, EightByteReal, real, double,  :,              out)
VARN(get, 2D, EightByteReal, real, double, `:,:',           out)
VARN(get, 3D, EightByteReal, real, double, `:,:,:',         out)
VARN(get, 4D, EightByteReal, real, double, `:,:,:,:',       out)
VARN(get, 5D, EightByteReal, real, double, `:,:,:,:,:',     out)
VARN(get, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   out)
VARN(get, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', out)

VARN(get, 1D, EightByteInt, integer, int8,  :,              out)
VARN(get, 2D, EightByteInt, integer, int8, `:,:',           out)
VARN(get, 3D, EightByteInt, integer, int8, `:,:,:',         out)
VARN(get, 4D, EightByteInt, integer, int8, `:,:,:,:',       out)
VARN(get, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     out)
VARN(get, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   out)
VARN(get, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', out)

!
! collective put APIs
!

VARN(put, 1D, OneByteInt, integer, int1,  :,              in, _all)
VARN(put, 2D, OneByteInt, integer, int1, `:,:',           in, _all)
VARN(put, 3D, OneByteInt, integer, int1, `:,:,:',         in, _all)
VARN(put, 4D, OneByteInt, integer, int1, `:,:,:,:',       in, _all)
VARN(put, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     in, _all)
VARN(put, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   in, _all)
VARN(put, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', in, _all)

VARN(put, 1D, TwoByteInt, integer, int2,  :,              INTENTV, _all)
VARN(put, 2D, TwoByteInt, integer, int2, `:,:',           INTENTV, _all)
VARN(put, 3D, TwoByteInt, integer, int2, `:,:,:',         INTENTV, _all)
VARN(put, 4D, TwoByteInt, integer, int2, `:,:,:,:',       INTENTV, _all)
VARN(put, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     INTENTV, _all)
VARN(put, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   INTENTV, _all)
VARN(put, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', INTENTV, _all)

VARN(put, 1D, FourByteInt, integer, int,  :,              INTENTV, _all)
VARN(put, 2D, FourByteInt, integer, int, `:,:',           INTENTV, _all)
VARN(put, 3D, FourByteInt, integer, int, `:,:,:',         INTENTV, _all)
VARN(put, 4D, FourByteInt, integer, int, `:,:,:,:',       INTENTV, _all)
VARN(put, 5D, FourByteInt, integer, int, `:,:,:,:,:',     INTENTV, _all)
VARN(put, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   INTENTV, _all)
VARN(put, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', INTENTV, _all)

VARN(put, 1D, FourByteReal, real,   real,  :,              INTENTV, _all)
VARN(put, 2D, FourByteReal, real,   real, `:,:',           INTENTV, _all)
VARN(put, 3D, FourByteReal, real,   real, `:,:,:',         INTENTV, _all)
VARN(put, 4D, FourByteReal, real,   real, `:,:,:,:',       INTENTV, _all)
VARN(put, 5D, FourByteReal, real,   real, `:,:,:,:,:',     INTENTV, _all)
VARN(put, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   INTENTV, _all)
VARN(put, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', INTENTV, _all)

VARN(put, 1D, EightByteReal, real, double,  :,              INTENTV, _all)
VARN(put, 2D, EightByteReal, real, double, `:,:',           INTENTV, _all)
VARN(put, 3D, EightByteReal, real, double, `:,:,:',         INTENTV, _all)
VARN(put, 4D, EightByteReal, real, double, `:,:,:,:',       INTENTV, _all)
VARN(put, 5D, EightByteReal, real, double, `:,:,:,:,:',     INTENTV, _all)
VARN(put, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   INTENTV, _all)
VARN(put, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', INTENTV, _all)

VARN(put, 1D, EightByteInt, integer, int8,  :,              INTENTV, _all)
VARN(put, 2D, EightByteInt, integer, int8, `:,:',           INTENTV, _all)
VARN(put, 3D, EightByteInt, integer, int8, `:,:,:',         INTENTV, _all)
VARN(put, 4D, EightByteInt, integer, int8, `:,:,:,:',       INTENTV, _all)
VARN(put, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     INTENTV, _all)
VARN(put, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   INTENTV, _all)
VARN(put, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', INTENTV, _all)

!
! collective get APIs
!

VARN(get, 1D, OneByteInt, integer, int1,  :,              out, _all)
VARN(get, 2D, OneByteInt, integer, int1, `:,:',           out, _all)
VARN(get, 3D, OneByteInt, integer, int1, `:,:,:',         out, _all)
VARN(get, 4D, OneByteInt, integer, int1, `:,:,:,:',       out, _all)
VARN(get, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     out, _all)
VARN(get, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   out, _all)
VARN(get, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', out, _all)

VARN(get, 1D, TwoByteInt, integer, int2,  :,              out, _all)
VARN(get, 2D, TwoByteInt, integer, int2, `:,:',           out, _all)
VARN(get, 3D, TwoByteInt, integer, int2, `:,:,:',         out, _all)
VARN(get, 4D, TwoByteInt, integer, int2, `:,:,:,:',       out, _all)
VARN(get, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     out, _all)
VARN(get, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   out, _all)
VARN(get, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', out, _all)

VARN(get, 1D, FourByteInt, integer, int,  :,              out, _all)
VARN(get, 2D, FourByteInt, integer, int, `:,:',           out, _all)
VARN(get, 3D, FourByteInt, integer, int, `:,:,:',         out, _all)
VARN(get, 4D, FourByteInt, integer, int, `:,:,:,:',       out, _all)
VARN(get, 5D, FourByteInt, integer, int, `:,:,:,:,:',     out, _all)
VARN(get, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   out, _all)
VARN(get, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', out, _all)

VARN(get, 1D, FourByteReal, real,   real,  :,              out, _all)
VARN(get, 2D, FourByteReal, real,   real, `:,:',           out, _all)
VARN(get, 3D, FourByteReal, real,   real, `:,:,:',         out, _all)
VARN(get, 4D, FourByteReal, real,   real, `:,:,:,:',       out, _all)
VARN(get, 5D, FourByteReal, real,   real, `:,:,:,:,:',     out, _all)
VARN(get, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   out, _all)
VARN(get, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', out, _all)

VARN(get, 1D, EightByteReal, real, double,  :,              out, _all)
VARN(get, 2D, EightByteReal, real, double, `:,:',           out, _all)
VARN(get, 3D, EightByteReal, real, double, `:,:,:',         out, _all)
VARN(get, 4D, EightByteReal, real, double, `:,:,:,:',       out, _all)
VARN(get, 5D, EightByteReal, real, double, `:,:,:,:,:',     out, _all)
VARN(get, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   out, _all)
VARN(get, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', out, _all)

VARN(get, 1D, EightByteInt, integer, int8,  :,              out, _all)
VARN(get, 2D, EightByteInt, integer, int8, `:,:',           out, _all)
VARN(get, 3D, EightByteInt, integer, int8, `:,:,:',         out, _all)
VARN(get, 4D, EightByteInt, integer, int8, `:,:,:,:',       out, _all)
VARN(get, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     out, _all)
VARN(get, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   out, _all)
VARN(get, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', out, _all)

!
! text variable
!

dnl
dnl TEXTVARN1(ncid, varid, values, num, start, count)
dnl
define(`TEXTVARN1',dnl
`dnl
   ! $1 a scalar of type character (len = *)
   function nf90mpi_$1_varn_text$3(ncid, varid, value, start)
     integer,                                        intent(in) :: ncid, varid
     character (len=*),                              intent($2) :: value
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), intent(in) :: start
     integer                                                    :: nf90mpi_$1_varn_text$3
 
     nf90mpi_$1_varn_text$3 = nfmpi_$1_var1_text$3(ncid, varid, start(:,1), value)
   end function nf90mpi_$1_varn_text$3
')dnl

TEXTVARN1(put, in)
TEXTVARN1(get, out)
TEXTVARN1(put, in,  _all)
TEXTVARN1(get, out, _all)

dnl
dnl TEXTVARN(ncid, varid, values, num, start, count)
dnl
define(`TEXTVARN',dnl
`dnl
   function nf90mpi_$1_varn_$2_text$6(ncid, varid, values, num, start, count)
     integer,                                                  intent(in) :: ncid, varid, num
     character (len=*),              dimension($3),            intent($5) :: values
     integer (kind=MPI_OFFSET_KIND), dimension(:,:),           intent(in) :: start
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), optional, intent(in) :: count
     integer                                                              :: nf90mpi_$1_varn_$2_text$6
     integer (kind=MPI_OFFSET_KIND), dimension(nf90_max_var_dims,num)     :: localCount
     integer                                                              :: numDims
 
     ! Set local arguments to default values
     numDims = size(start(:,1))
     localCount(1:numDims,1:num) = 1
     if (present(count)) localCount(1:numDims,1:num) = count(1:numDims,1:num)
     nf90mpi_$1_varn_$2_text$6 = nfmpi_$1_varn_text$6(ncid, varid, num, start, &
                                                      localCount(1:numDims,1:num), values($4))
   end function nf90mpi_$1_varn_$2_text$6
')dnl

TEXTVARN(put, 1D,  :,               1,              in)
TEXTVARN(put, 2D, `:,:',           `1,1',           in)
TEXTVARN(put, 3D, `:,:,:',         `1,1,1',         in)
TEXTVARN(put, 4D, `:,:,:,:',       `1,1,1,1',       in)
TEXTVARN(put, 5D, `:,:,:,:,:',     `1,1,1,1,1',     in)
TEXTVARN(put, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   in)
TEXTVARN(put, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', in)

TEXTVARN(get, 1D,  :,               1,              out)
TEXTVARN(get, 2D, `:,:',           `1,1',           out)
TEXTVARN(get, 3D, `:,:,:',         `1,1,1',         out)
TEXTVARN(get, 4D, `:,:,:,:',       `1,1,1,1',       out)
TEXTVARN(get, 5D, `:,:,:,:,:',     `1,1,1,1,1',     out)
TEXTVARN(get, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   out)
TEXTVARN(get, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', out)

!
! Collective APIs
!

TEXTVARN(put, 1D,  :,               1,              in, _all)
TEXTVARN(put, 2D, `:,:',           `1,1',           in, _all)
TEXTVARN(put, 3D, `:,:,:',         `1,1,1',         in, _all)
TEXTVARN(put, 4D, `:,:,:,:',       `1,1,1,1',       in, _all)
TEXTVARN(put, 5D, `:,:,:,:,:',     `1,1,1,1,1',     in, _all)
TEXTVARN(put, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   in, _all)
TEXTVARN(put, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', in, _all)

TEXTVARN(get, 1D,  :,               1,              out, _all)
TEXTVARN(get, 2D, `:,:',           `1,1',           out, _all)
TEXTVARN(get, 3D, `:,:,:',         `1,1,1',         out, _all)
TEXTVARN(get, 4D, `:,:,:,:',       `1,1,1,1',       out, _all)
TEXTVARN(get, 5D, `:,:,:,:,:',     `1,1,1,1,1',     out, _all)
TEXTVARN(get, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   out, _all)
TEXTVARN(get, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', out, _all)

!
! Nonblocking APIs
!

dnl
dnl IVARN1
dnl
define(`IVARN1',dnl
`dnl
   ! $1 a scalar of type $4 (kind=$3)
   function nf90mpi_$1_varn_$3(ncid, varid, value, req, start)
     integer,                                        intent(in) :: ncid, varid
     $4 (kind=$3),                                   intent($2) :: value
     integer,                                        intent(out):: req
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), intent(in) :: start
     integer                                                    :: nf90mpi_$1_varn_$3
 
     nf90mpi_$1_varn_$3 = nfmpi_$1_var1_$5(ncid, varid, start(:,1), value, req)
   end function nf90mpi_$1_varn_$3
')dnl

IVARN1(iput, in,    OneByteInt,    integer, int1)
IVARN1(iput, INTENTV, TwoByteInt,    integer, int2)
IVARN1(iput, INTENTV, FourByteInt,   integer, int)
IVARN1(iput, INTENTV, FourByteReal,  real,    real)
IVARN1(iput, INTENTV, EightByteReal, real,    double)
IVARN1(iput, INTENTV, EightByteInt,  integer, int8)

IVARN1(iget, out,   OneByteInt,    integer, int1)
IVARN1(iget, out,   TwoByteInt,    integer, int2)
IVARN1(iget, out,   FourByteInt,   integer, int)
IVARN1(iget, out,   FourByteReal,  real,    real)
IVARN1(iget, out,   EightByteReal, real,    double)
IVARN1(iget, out,   EightByteInt,  integer, int8)

IVARN1(bput, in,    OneByteInt,    integer, int1)
IVARN1(bput, INTENTV, TwoByteInt,    integer, int2)
IVARN1(bput, INTENTV, FourByteInt,   integer, int)
IVARN1(bput, INTENTV, FourByteReal,  real,    real)
IVARN1(bput, INTENTV, EightByteReal, real,    double)
IVARN1(bput, INTENTV, EightByteInt,  integer, int8)

dnl
dnl IVARN(ncid, varid, values, num, start, count)
dnl
define(`IVARN',dnl
`dnl
   function nf90mpi_$1_varn_$2_$3(ncid, varid, values, req, num, start, count)
     integer,                                                  intent(in) :: ncid, varid, num
     $4 (kind=$3),                   dimension($6),            intent($7) :: values
     integer,                                                  intent(out):: req
     integer (kind=MPI_OFFSET_KIND), dimension(:,:),           intent(in) :: start
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), optional, intent(in) :: count
     integer                                                              :: nf90mpi_$1_varn_$2_$3
     integer (kind=MPI_OFFSET_KIND), dimension(nf90_max_var_dims,num)     :: localCount
     integer                                                              :: numDims
 
     ! Set local arguments to default values
     numDims = size(start(:,1))
     localCount(1:numDims,1:num) = 1
     if (present(count)) localCount(1:numDims,1:num) = count(1:numDims,1:num)
     nf90mpi_$1_varn_$2_$3 = nfmpi_$1_varn_$5(ncid, varid, num, start, &
                                              localCount(1:numDims,1:num), values, req)
   end function nf90mpi_$1_varn_$2_$3
')dnl

!
! put APIs
!

IVARN(iput, 1D, OneByteInt, integer, int1,  :,              in)
IVARN(iput, 2D, OneByteInt, integer, int1, `:,:',           in)
IVARN(iput, 3D, OneByteInt, integer, int1, `:,:,:',         in)
IVARN(iput, 4D, OneByteInt, integer, int1, `:,:,:,:',       in)
IVARN(iput, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     in)
IVARN(iput, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   in)
IVARN(iput, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', in)

IVARN(iput, 1D, TwoByteInt, integer, int2,  :,              INTENTV)
IVARN(iput, 2D, TwoByteInt, integer, int2, `:,:',           INTENTV)
IVARN(iput, 3D, TwoByteInt, integer, int2, `:,:,:',         INTENTV)
IVARN(iput, 4D, TwoByteInt, integer, int2, `:,:,:,:',       INTENTV)
IVARN(iput, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     INTENTV)
IVARN(iput, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   INTENTV)
IVARN(iput, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', INTENTV)

IVARN(iput, 1D, FourByteInt, integer, int,  :,              INTENTV)
IVARN(iput, 2D, FourByteInt, integer, int, `:,:',           INTENTV)
IVARN(iput, 3D, FourByteInt, integer, int, `:,:,:',         INTENTV)
IVARN(iput, 4D, FourByteInt, integer, int, `:,:,:,:',       INTENTV)
IVARN(iput, 5D, FourByteInt, integer, int, `:,:,:,:,:',     INTENTV)
IVARN(iput, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   INTENTV)
IVARN(iput, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', INTENTV)

IVARN(iput, 1D, FourByteReal, real,   real,  :,              INTENTV)
IVARN(iput, 2D, FourByteReal, real,   real, `:,:',           INTENTV)
IVARN(iput, 3D, FourByteReal, real,   real, `:,:,:',         INTENTV)
IVARN(iput, 4D, FourByteReal, real,   real, `:,:,:,:',       INTENTV)
IVARN(iput, 5D, FourByteReal, real,   real, `:,:,:,:,:',     INTENTV)
IVARN(iput, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   INTENTV)
IVARN(iput, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', INTENTV)

IVARN(iput, 1D, EightByteReal, real, double,  :,              INTENTV)
IVARN(iput, 2D, EightByteReal, real, double, `:,:',           INTENTV)
IVARN(iput, 3D, EightByteReal, real, double, `:,:,:',         INTENTV)
IVARN(iput, 4D, EightByteReal, real, double, `:,:,:,:',       INTENTV)
IVARN(iput, 5D, EightByteReal, real, double, `:,:,:,:,:',     INTENTV)
IVARN(iput, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   INTENTV)
IVARN(iput, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', INTENTV)

IVARN(iput, 1D, EightByteInt, integer, int8,  :,              INTENTV)
IVARN(iput, 2D, EightByteInt, integer, int8, `:,:',           INTENTV)
IVARN(iput, 3D, EightByteInt, integer, int8, `:,:,:',         INTENTV)
IVARN(iput, 4D, EightByteInt, integer, int8, `:,:,:,:',       INTENTV)
IVARN(iput, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     INTENTV)
IVARN(iput, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   INTENTV)
IVARN(iput, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', INTENTV)

!
! get APIs
!

IVARN(iget, 1D, OneByteInt, integer, int1,  :,              out)
IVARN(iget, 2D, OneByteInt, integer, int1, `:,:',           out)
IVARN(iget, 3D, OneByteInt, integer, int1, `:,:,:',         out)
IVARN(iget, 4D, OneByteInt, integer, int1, `:,:,:,:',       out)
IVARN(iget, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     out)
IVARN(iget, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   out)
IVARN(iget, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', out)

IVARN(iget, 1D, TwoByteInt, integer, int2,  :,              out)
IVARN(iget, 2D, TwoByteInt, integer, int2, `:,:',           out)
IVARN(iget, 3D, TwoByteInt, integer, int2, `:,:,:',         out)
IVARN(iget, 4D, TwoByteInt, integer, int2, `:,:,:,:',       out)
IVARN(iget, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     out)
IVARN(iget, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   out)
IVARN(iget, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', out)

IVARN(iget, 1D, FourByteInt, integer, int,  :,              out)
IVARN(iget, 2D, FourByteInt, integer, int, `:,:',           out)
IVARN(iget, 3D, FourByteInt, integer, int, `:,:,:',         out)
IVARN(iget, 4D, FourByteInt, integer, int, `:,:,:,:',       out)
IVARN(iget, 5D, FourByteInt, integer, int, `:,:,:,:,:',     out)
IVARN(iget, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   out)
IVARN(iget, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', out)

IVARN(iget, 1D, FourByteReal, real,   real,  :,              out)
IVARN(iget, 2D, FourByteReal, real,   real, `:,:',           out)
IVARN(iget, 3D, FourByteReal, real,   real, `:,:,:',         out)
IVARN(iget, 4D, FourByteReal, real,   real, `:,:,:,:',       out)
IVARN(iget, 5D, FourByteReal, real,   real, `:,:,:,:,:',     out)
IVARN(iget, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   out)
IVARN(iget, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', out)

IVARN(iget, 1D, EightByteReal, real, double,  :,              out)
IVARN(iget, 2D, EightByteReal, real, double, `:,:',           out)
IVARN(iget, 3D, EightByteReal, real, double, `:,:,:',         out)
IVARN(iget, 4D, EightByteReal, real, double, `:,:,:,:',       out)
IVARN(iget, 5D, EightByteReal, real, double, `:,:,:,:,:',     out)
IVARN(iget, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   out)
IVARN(iget, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', out)

IVARN(iget, 1D, EightByteInt, integer, int8,  :,              out)
IVARN(iget, 2D, EightByteInt, integer, int8, `:,:',           out)
IVARN(iget, 3D, EightByteInt, integer, int8, `:,:,:',         out)
IVARN(iget, 4D, EightByteInt, integer, int8, `:,:,:,:',       out)
IVARN(iget, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     out)
IVARN(iget, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   out)
IVARN(iget, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', out)

!
! bput APIs
!

IVARN(bput, 1D, OneByteInt, integer, int1,  :,              in)
IVARN(bput, 2D, OneByteInt, integer, int1, `:,:',           in)
IVARN(bput, 3D, OneByteInt, integer, int1, `:,:,:',         in)
IVARN(bput, 4D, OneByteInt, integer, int1, `:,:,:,:',       in)
IVARN(bput, 5D, OneByteInt, integer, int1, `:,:,:,:,:',     in)
IVARN(bput, 6D, OneByteInt, integer, int1, `:,:,:,:,:,:',   in)
IVARN(bput, 7D, OneByteInt, integer, int1, `:,:,:,:,:,:,:', in)

IVARN(bput, 1D, TwoByteInt, integer, int2,  :,              INTENTV)
IVARN(bput, 2D, TwoByteInt, integer, int2, `:,:',           INTENTV)
IVARN(bput, 3D, TwoByteInt, integer, int2, `:,:,:',         INTENTV)
IVARN(bput, 4D, TwoByteInt, integer, int2, `:,:,:,:',       INTENTV)
IVARN(bput, 5D, TwoByteInt, integer, int2, `:,:,:,:,:',     INTENTV)
IVARN(bput, 6D, TwoByteInt, integer, int2, `:,:,:,:,:,:',   INTENTV)
IVARN(bput, 7D, TwoByteInt, integer, int2, `:,:,:,:,:,:,:', INTENTV)

IVARN(bput, 1D, FourByteInt, integer, int,  :,              INTENTV)
IVARN(bput, 2D, FourByteInt, integer, int, `:,:',           INTENTV)
IVARN(bput, 3D, FourByteInt, integer, int, `:,:,:',         INTENTV)
IVARN(bput, 4D, FourByteInt, integer, int, `:,:,:,:',       INTENTV)
IVARN(bput, 5D, FourByteInt, integer, int, `:,:,:,:,:',     INTENTV)
IVARN(bput, 6D, FourByteInt, integer, int, `:,:,:,:,:,:',   INTENTV)
IVARN(bput, 7D, FourByteInt, integer, int, `:,:,:,:,:,:,:', INTENTV)

IVARN(bput, 1D, FourByteReal, real,   real,  :,              INTENTV)
IVARN(bput, 2D, FourByteReal, real,   real, `:,:',           INTENTV)
IVARN(bput, 3D, FourByteReal, real,   real, `:,:,:',         INTENTV)
IVARN(bput, 4D, FourByteReal, real,   real, `:,:,:,:',       INTENTV)
IVARN(bput, 5D, FourByteReal, real,   real, `:,:,:,:,:',     INTENTV)
IVARN(bput, 6D, FourByteReal, real,   real, `:,:,:,:,:,:',   INTENTV)
IVARN(bput, 7D, FourByteReal, real,   real, `:,:,:,:,:,:,:', INTENTV)

IVARN(bput, 1D, EightByteReal, real, double,  :,              INTENTV)
IVARN(bput, 2D, EightByteReal, real, double, `:,:',           INTENTV)
IVARN(bput, 3D, EightByteReal, real, double, `:,:,:',         INTENTV)
IVARN(bput, 4D, EightByteReal, real, double, `:,:,:,:',       INTENTV)
IVARN(bput, 5D, EightByteReal, real, double, `:,:,:,:,:',     INTENTV)
IVARN(bput, 6D, EightByteReal, real, double, `:,:,:,:,:,:',   INTENTV)
IVARN(bput, 7D, EightByteReal, real, double, `:,:,:,:,:,:,:', INTENTV)

IVARN(bput, 1D, EightByteInt, integer, int8,  :,              INTENTV)
IVARN(bput, 2D, EightByteInt, integer, int8, `:,:',           INTENTV)
IVARN(bput, 3D, EightByteInt, integer, int8, `:,:,:',         INTENTV)
IVARN(bput, 4D, EightByteInt, integer, int8, `:,:,:,:',       INTENTV)
IVARN(bput, 5D, EightByteInt, integer, int8, `:,:,:,:,:',     INTENTV)
IVARN(bput, 6D, EightByteInt, integer, int8, `:,:,:,:,:,:',   INTENTV)
IVARN(bput, 7D, EightByteInt, integer, int8, `:,:,:,:,:,:,:', INTENTV)

!
! text variable
!

dnl
dnl ITEXTVARN1
dnl
define(`ITEXTVARN1',dnl
`dnl
   ! $1 a scalar of type character (len = *)
   function nf90mpi_$1_varn_text(ncid, varid, value, req, start)
     integer,                                        intent(in) :: ncid, varid
     character (len = *),                            intent($2) :: value
     integer,                                        intent(out):: req
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), intent(in) :: start
     integer                                                    :: nf90mpi_$1_varn_text
 
     nf90mpi_$1_varn_text = nfmpi_$1_var1_text(ncid, varid, start(:,1), value, req)
   end function nf90mpi_$1_varn_text
')dnl

ITEXTVARN1(iput, in)
ITEXTVARN1(iget, out)
ITEXTVARN1(bput, in)

dnl
dnl ITEXTVARN(ncid, varid, values, num, start, count)
dnl
define(`ITEXTVARN',dnl
`dnl
   function nf90mpi_$1_varn_$2_text(ncid, varid, values, req, num, start, count)
     integer,                                                  intent(in) :: ncid, varid, num
     character (len=*),              dimension($3),            intent($5) :: values
     integer,                                                  intent(out):: req
     integer (kind=MPI_OFFSET_KIND), dimension(:,:),           intent(in) :: start
     integer (kind=MPI_OFFSET_KIND), dimension(:,:), optional, intent(in) :: count
     integer                                                              :: nf90mpi_$1_varn_$2_text
     integer (kind=MPI_OFFSET_KIND), dimension(nf90_max_var_dims,num)     :: localCount
     integer                                                              :: numDims
 
     ! Set local arguments to default values
     numDims = size(start(:,1))
     localCount(1:numDims,1:num) = 1
     if (present(count)) localCount(1:numDims,1:num) = count(1:numDims,1:num)
     nf90mpi_$1_varn_$2_text = nfmpi_$1_varn_text(ncid, varid, num, start, &
                                                  localCount(1:numDims,1:num), values($4), req)
   end function nf90mpi_$1_varn_$2_text
')dnl

ITEXTVARN(iput, 1D,  :,               1,              in)
ITEXTVARN(iput, 2D, `:,:',           `1,1',           in)
ITEXTVARN(iput, 3D, `:,:,:',         `1,1,1',         in)
ITEXTVARN(iput, 4D, `:,:,:,:',       `1,1,1,1',       in)
ITEXTVARN(iput, 5D, `:,:,:,:,:',     `1,1,1,1,1',     in)
ITEXTVARN(iput, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   in)
ITEXTVARN(iput, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', in)

ITEXTVARN(iget, 1D,  :,               1,              out)
ITEXTVARN(iget, 2D, `:,:',           `1,1',           out)
ITEXTVARN(iget, 3D, `:,:,:',         `1,1,1',         out)
ITEXTVARN(iget, 4D, `:,:,:,:',       `1,1,1,1',       out)
ITEXTVARN(iget, 5D, `:,:,:,:,:',     `1,1,1,1,1',     out)
ITEXTVARN(iget, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   out)
ITEXTVARN(iget, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', out)

ITEXTVARN(bput, 1D,  :,               1,              in)
ITEXTVARN(bput, 2D, `:,:',           `1,1',           in)
ITEXTVARN(bput, 3D, `:,:,:',         `1,1,1',         in)
ITEXTVARN(bput, 4D, `:,:,:,:',       `1,1,1,1',       in)
ITEXTVARN(bput, 5D, `:,:,:,:,:',     `1,1,1,1,1',     in)
ITEXTVARN(bput, 6D, `:,:,:,:,:,:',   `1,1,1,1,1,1',   in)
ITEXTVARN(bput, 7D, `:,:,:,:,:,:,:', `1,1,1,1,1,1,1', in)

