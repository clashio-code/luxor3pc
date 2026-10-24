/* Luxor 3 native ARM64 track prototype. SDL2 + OpenGL ES 2.0.
 * Parses the game's text .LX3TRK file and renders its actual Sections/Verts.
 * This is an engine milestone, NOT a complete recreation of Luxor 3.
 */
#include <SDL.h>
#include <GLES2/gl2.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_VERTS 4096
#define MAX_SECTIONS 128
#define MAX_SECTION_VERTS 1024
#define PI 3.14159265358979323846f

typedef struct { float x,y,z; } Point3;
typedef struct { char name[64]; int ids[MAX_SECTION_VERTS]; int count; } Section;
typedef struct { float x,y,r,g,b,a; } Vtx;
static Point3 points[MAX_VERTS]; static int point_count;
static Section sections[MAX_SECTIONS]; static int section_count;
static GLuint program, vbo; static GLint a_pos, a_col;
static int ww=640,hh=480,running=1,selected=0; static float t=0;
static float minx,maxx,miny,maxy,scale,offx,offy;
static const char *VS="attribute vec2 a_pos; attribute vec4 a_col; varying vec4 v_col; void main(){gl_Position=vec4(a_pos,0.0,1.0);v_col=a_col;}";
static const char *FS="precision mediump float; varying vec4 v_col; void main(){gl_FragColor=v_col;}";
static GLuint shader(GLenum type,const char *src){GLuint s=glCreateShader(type);glShaderSource(s,1,&src,NULL);glCompileShader(s);GLint ok=0;glGetShaderiv(s,GL_COMPILE_STATUS,&ok);if(!ok){char b[1024]={0};glGetShaderInfoLog(s,sizeof b,NULL,b);fprintf(stderr,"shader: %s\n",b);glDeleteShader(s);return 0;}return s;}
static int init_gl(void){GLuint vs=shader(GL_VERTEX_SHADER,VS),fs=shader(GL_FRAGMENT_SHADER,FS);if(!vs||!fs)return 0;program=glCreateProgram();glAttachShader(program,vs);glAttachShader(program,fs);glBindAttribLocation(program,0,"a_pos");glBindAttribLocation(program,1,"a_col");glLinkProgram(program);glDeleteShader(vs);glDeleteShader(fs);GLint ok=0;glGetProgramiv(program,GL_LINK_STATUS,&ok);if(!ok)return 0;a_pos=glGetAttribLocation(program,"a_pos");a_col=glGetAttribLocation(program,"a_col");glGenBuffers(1,&vbo);return 1;}
static int parse_track(const char *path){
 FILE *f=fopen(path,"r"); if(!f){fprintf(stderr,"Cannot open track: %s\n",path);return 0;}
 char line[512]; int mode=0;
 while(fgets(line,sizeof line,f)){
  char *p=line; while(*p==' '||*p=='\t')p++;
  if(!strncmp(p,"Verts",5)){mode=1;continue;}
  if(!strncmp(p,"Sections",8)){mode=2;continue;}
  if(!strncmp(p,"Reflectors",10)||!strncmp(p,"NamedPoints",11)){mode=0;continue;}
  if(mode==1){float x,y,z;if(sscanf(p,"Vert %f %f %f",&x,&y,&z)==3&&point_count<MAX_VERTS)points[point_count++]=(Point3){x,y,z};}
  else if(mode==2 && *p=='"' && section_count<MAX_SECTIONS){
   char name[64]={0}; if(sscanf(p,"\"%63[^\"]\"",name)!=1)continue;
   Section *q=&sections[section_count++];memset(q,0,sizeof *q);strncpy(q->name,name,sizeof q->name-1);
   char *a=strchr(p,'{'); if(!a)continue; a++;
   while(*a&&*a!='}'&&q->count<MAX_SECTION_VERTS){while(*a==' '||*a=='\t')a++;if(*a>='0'&&*a<='9'){char *e;long id=strtol(a,&e,10);if(e==a)break;q->ids[q->count++]=(int)id;a=e;}else a++;}
  }
 }
 fclose(f);
 if(point_count<2||section_count<1){fprintf(stderr,"Track parse failed: verts=%d sections=%d\n",point_count,section_count);return 0;}
 minx=maxx=points[0].x;miny=maxy=points[0].y;
 for(int i=1;i<point_count;i++){if(points[i].x<minx)minx=points[i].x;if(points[i].x>maxx)maxx=points[i].x;if(points[i].y<miny)miny=points[i].y;if(points[i].y>maxy)maxy=points[i].y;}
 float sx=1.65f/(maxx-minx),sy=1.55f/(maxy-miny);scale=sx<sy?sx:sy;offx=-(minx+maxx)*0.5f;offy=-(miny+maxy)*0.5f;
 fprintf(stderr,"[track] loaded %s: %d vertices, %d sections; bounds %.1f..%.1f / %.1f..%.1f\n",path,point_count,section_count,minx,maxx,miny,maxy);return 1;
}
static void xy(Point3 p,float *x,float *y){*x=(p.x+offx)*scale;*y=(p.y+offy)*scale;}
static void add(Vtx *v,int *n,float x,float y,float r,float g,float b){if(*n>=16000)return;v[*n]=(Vtx){x,y,r,g,b,1};(*n)++;}
static void draw(void){Vtx v[16000];int n=0;/* dark backdrop */add(v,&n,-1,-1,.025f,.035f,.11f);add(v,&n,1,-1,.025f,.035f,.11f);add(v,&n,1,1,.045f,.035f,.15f);add(v,&n,-1,-1,.025f,.035f,.11f);add(v,&n,1,1,.045f,.035f,.15f);add(v,&n,-1,1,.045f,.035f,.15f);
 for(int s=0;s<section_count;s++){Section *q=&sections[s];if(q->count<2)continue;for(int j=0;j<q->count;j++){int id=q->ids[j];if(id<0||id>=point_count)continue;float x,y;xy(points[id],&x,&y);float r=.30f,g=.19f,b=.09f;if(strstr(q->name,"main")){r=.78f;g=.47f;b=.20f;}if(!strcmp(q->name,"initial")||!strcmp(q->name,"final")){r=.98f;g=.70f;b=.30f;}add(v,&n,x,y,r,g,b);}}
 /* moving test marble along selected section */
 if(section_count){Section *q=&sections[selected%section_count];if(q->count>1){float f=fmodf(t*.12f,1.0f)*(q->count-1);int i=(int)f;float a=f-i;if(i>=q->count-1)i=q->count-2;int p0=q->ids[i],p1=q->ids[i+1];if(p0>=0&&p0<point_count&&p1>=0&&p1<point_count){float x0,y0,x1,y1;xy(points[p0],&x0,&y0);xy(points[p1],&x1,&y1);float x=x0+(x1-x0)*a,y=y0+(y1-y0)*a;for(int k=0;k<24;k++){float a0=2*PI*k/24,a1=2*PI*(k+1)/24;add(v,&n,x,y,.95f,.14f,.10f);add(v,&n,x+cosf(a0)*.018f,y+sinf(a0)*.018f,.95f,.14f,.10f);add(v,&n,x+cosf(a1)*.018f,y+sinf(a1)*.018f,.95f,.14f,.10f);}}}}
 glViewport(0,0,ww,hh);glClearColor(.02f,.025f,.07f,1);glClear(GL_COLOR_BUFFER_BIT);glUseProgram(program);glBindBuffer(GL_ARRAY_BUFFER,vbo);glBufferData(GL_ARRAY_BUFFER,n*sizeof(Vtx),v,GL_DYNAMIC_DRAW);glEnableVertexAttribArray(a_pos);glVertexAttribPointer(a_pos,2,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)0);glEnableVertexAttribArray(a_col);glVertexAttribPointer(a_col,4,GL_FLOAT,GL_FALSE,sizeof(Vtx),(void*)(2*sizeof(float)));/* first 6 vertices are background triangles */glDrawArrays(GL_TRIANGLES,0,6);int cursor=6;for(int s=0;s<section_count;s++){int count=sections[s].count;if(count>1&&cursor+count<=n){glDrawArrays(GL_LINE_STRIP,cursor,count);cursor+=count;}}/* test marble triangles are included in tail; draw all triangle tail */if(cursor<n)glDrawArrays(GL_TRIANGLES,cursor,n-cursor);glDisableVertexAttribArray(a_pos);glDisableVertexAttribArray(a_col);}
int main(int argc,char **argv){const char *track=argc>1?argv[1]:"assets/path.LX3TRK";if(!parse_track(track))return 2;if(SDL_Init(SDL_INIT_VIDEO|SDL_INIT_GAMECONTROLLER|SDL_INIT_EVENTS)){fprintf(stderr,"SDL_Init: %s\n",SDL_GetError());return 3;}SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK,SDL_GL_CONTEXT_PROFILE_ES);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION,2);SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION,0);SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER,1);SDL_Window *w=SDL_CreateWindow("Luxor 3 Native Track Prototype",SDL_WINDOWPOS_CENTERED,SDL_WINDOWPOS_CENTERED,ww,hh,SDL_WINDOW_OPENGL|SDL_WINDOW_RESIZABLE|SDL_WINDOW_SHOWN);if(!w){fprintf(stderr,"SDL window: %s\n",SDL_GetError());return 4;}SDL_GLContext c=SDL_GL_CreateContext(w);if(!c){fprintf(stderr,"GLES context: %s\n",SDL_GetError());return 5;}SDL_GL_SetSwapInterval(1);if(!init_gl())return 6;fprintf(stderr,"[GL] renderer=%s version=%s\n",glGetString(GL_RENDERER),glGetString(GL_VERSION));Uint64 start=SDL_GetPerformanceCounter();while(running){SDL_Event e;while(SDL_PollEvent(&e)){if(e.type==SDL_QUIT)running=0;if(e.type==SDL_WINDOWEVENT&&e.window.event==SDL_WINDOWEVENT_SIZE_CHANGED){ww=e.window.data1;hh=e.window.data2;}if(e.type==SDL_KEYDOWN){if(e.key.keysym.sym==SDLK_ESCAPE)running=0;if(e.key.keysym.sym==SDLK_RIGHT)selected=(selected+1)%section_count;if(e.key.keysym.sym==SDLK_LEFT)selected=(selected+section_count-1)%section_count;}if(e.type==SDL_CONTROLLERBUTTONDOWN){if(e.cbutton.button==SDL_CONTROLLER_BUTTON_START)running=0;if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_RIGHT)selected=(selected+1)%section_count;if(e.cbutton.button==SDL_CONTROLLER_BUTTON_DPAD_LEFT)selected=(selected+section_count-1)%section_count;}}Uint64 now=SDL_GetPerformanceCounter();t=(float)((double)(now-start)/SDL_GetPerformanceFrequency());int dw,dh;SDL_GL_GetDrawableSize(w,&dw,&dh);if(dw>0&&dh>0){ww=dw;hh=dh;}draw();SDL_GL_SwapWindow(w);}glDeleteBuffers(1,&vbo);glDeleteProgram(program);SDL_GL_DeleteContext(c);SDL_DestroyWindow(w);SDL_Quit();return 0;}
