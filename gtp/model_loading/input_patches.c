//
// The file input.c  -- Juhana Kouhia, jk87377@cs.tut.fi, Oct. 25, 1991
//
// Load_patch(char *filename, int *patches, *verticies );  
//   A sample program to read Bezier patches in.
//   Returns count of patches and verticies.
// User defined subroutines:
//   ADD A POINT TO THE VERTEX ARRAY
//   B_point(ii, x, y, z);
//     int ii; double x, y, z;
//     Defines one point with index number ii.
//   B_patch(ii, a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p);
//     int ii, a, b, ..., p;
//     Defines one Bezier patch with index number ii,
//     indexes to points are in a, b, c, ..., p.
//

void Load_patch( char *filename, int *patches, int *verticies )
{
  int ii;
  float x,y,z;
  int a,b,c,d,e,f,g,h,i,j,k,l,m,n,o,p;

  FILE *fp;

  if (!(fp = fopen(filename,"r"))) {
    fprintf(stderr,"Load_patch: Can't open %s\n",filename);
    exit(1);
  }

  (void)fscanf(fp,"%i\n",patches);
  for (ii = 0; ii < *patches; ii++) {
    (void)fscanf(fp,"%i, %i, %i, %i,",&a,&b,&c,&d);
    (void)fscanf(fp,"%i, %i, %i, %i,",&e,&f,&g,&h);
    (void)fscanf(fp,"%i, %i, %i, %i,",&i,&j,&k,&l);
    (void)fscanf(fp,"%i, %i, %i, %i\n",&m,&n,&o,&p);
    B_patch(ii, a,b,c,d, e,f,g,h, i,j,k,l, m,n,o,p); // INDICES to points, that were loaded in B_point.
  }
  (void)fscanf(fp,"%i\n",verticies);
  for (ii = 1; ii <= *verticies; ii++) {
    (void)fscanf(fp,"%f, %f, %f\n",&x,&y,&z);
    B_point(ii, (double)x,(double)y,(double)z);
  }
}
