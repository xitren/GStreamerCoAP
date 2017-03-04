struct img_present{
	int color_type;
	int mid_brightness;
	int width;
	int height;
	int precision;
	unsigned char ***source;
	unsigned char ** bit_matrix;
	int ** encryption_x;
	int ** encryption_y;
	int timer;
};
struct img_massive{
	int n;
	img_present** imgs;
};

img_present* createImgPresent(int _width, int _height, int _precision, int _color_type, int _timer, unsigned char *_source);
img_present* openFromFileImgPresent(char *filename);
int saveToFileImgPresent(char *filename, img_present *img);
int destroyImgPresent(img_present *img);

int shapeFilter(img_present *img);
int degrade(img_present *img);
int encrypt_x(img_present *img);
int encrypt_y(img_present *img);
img_present*  clone(img_present *img);
img_present*  make_precendent(img_present *img,int x1,int y1,int x2,int y2,int _timer);

img_present*  discrete(img_present *encryption, float divider);
bool compare(img_present *encryption1, img_present *encryption2);
bool insider(img_present *encryption1, img_present *encryption2);
bool check_in_pos(img_present *encryption1, int x, int y, img_present *encryption2, int precision, bool fast);
void print_img(img_present *img);
void print_encrypt(img_present *img);

bool timer_check(img_present *img);
int timer_dec(img_present *img);
//parallel functions
inline void _p_translateLine(img_present *img, unsigned char *_source, int column);
