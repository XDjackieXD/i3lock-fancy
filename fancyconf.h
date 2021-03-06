#define FILL_CIRCLE
#define DRAW_SEPERATOR
//#define IGNORE_SCALING

/* define what texts to show */
#define DRAW_TEXT_AUTH_VERIFY
#define DRAW_TEXT_AUTH_LOCK
#define DRAW_TEXT_AUTH_WRONG
#define DRAW_TEXT_LOCK_FAILED
#define DRAW_TEXT_NOTHING_TO_DELETE

/* define button sizes */
#define BUTTON_RADIUS	90
#define BUTTON_SPACE	(BUTTON_RADIUS + 5)
#define BUTTON_CENTER	(BUTTON_RADIUS + 5)
#define BUTTON_DIAMETER	(2 * BUTTON_SPACE)




/* Button highlight color while typing */
#define BUTTON_TYPE_R		51.0
#define BUTTON_TYPE_G		219.0
#define BUTTON_TYPE_B		0.0

/* Button highlight color while deleting */
#define BUTTON_DELETE_R		219.0
#define BUTTON_DELETE_G		51.0
#define BUTTON_DELETE_B		0.0

/* Seperator between button segments */
#define BUTTON_SEPERATOR_R	0.0
#define BUTTON_SEPERATOR_G	0.0
#define BUTTON_SEPERATOR_B	0.0


/* Seperator between stroke and fill */
#define FILL_SEPERATOR_R	0.0
#define FILL_SEPERATOR_G	0.0
#define FILL_SEPERATOR_B	0.0
#define FILL_SEPERATOR_WIDTH	2.0


/* size and color of all texts except attempts coutner */
#define TEXT_SIZE		28.0
#define TEXT_R			0.0
#define TEXT_G			0.0
#define TEXT_B			0.0

/* size and color of failed attempts counter */
#define TEXT_ATTEMPTS_SIZE	32.0
#define TEXT_ATTEMPTS_R		255.0
#define TEXT_ATTEMPTS_G		0.0
#define TEXT_ATTEMPTS_B		0.0


/* Inner circle fill while verifying */
#define FILL_AUTH_VERIFY_R	0.0
#define FILL_AUTH_VERIFY_G	114.0
#define FILL_AUTH_VERIFY_B	255.0
#define FILL_AUTH_VERIFY_A	0.75

/* Inner circle fill while locking */
#define FILL_AUTH_LOCK_R		0.0
#define FILL_AUTH_LOCK_G		114.0
#define FILL_AUTH_LOCK_B		255.0
#define FILL_AUTH_LOCK_A		0.75

/* Inner circle fill after wrong password verification */
#define FILL_AUTH_WRONG_R	250.0
#define FILL_AUTH_WRONG_G	0.0
#define FILL_AUTH_WRONG_B	0.0
#define FILL_AUTH_WRONG_A	0.75

/* Inner circle fill when locking of the screen failed */
#define FILL_LOCK_FAILED_R	250.0
#define FILL_LOCK_FAILED_G	0.0
#define FILL_LOCK_FAILED_B	0.0
#define FILL_LOCK_FAILED_A	0.75

/* Inner circle fill color when there is nothing to delete */
#define FILL_NOTHING_TO_DELETE_R    250.0
#define FILL_NOTHING_TO_DELETE_G    0.0
#define FILL_NOTHING_TO_DELETE_B    0.0
#define FILL_NOTHING_TO_DELETE_A    0.75

/* Inner circle fill while idle */
#define FILL_DEFAULT_R		0.0
#define FILL_DEFAULT_G		0.0
#define FILL_DEFAULT_B		0.0
#define FILL_DEFAULT_A		0.75


/* Outer stroke color while verifying */
#define STROKE_AUTH_VERIFY_R	51.0
#define STROKE_AUTH_VERIFY_G	0.0
#define STROKE_AUTH_VERIFY_B	250.0

/* Outer stroke color while locking */
#define STROKE_AUTH_LOCK_R	51.0
#define STROKE_AUTH_LOCK_G	0.0
#define STROKE_AUTH_LOCK_B	250.0

/* Outer circle color after wrong password verification */
#define STROKE_AUTH_WRONG_R	125.0
#define STROKE_AUTH_WRONG_G	51.0
#define STROKE_AUTH_WRONG_B	0.0

/* Outer circle color when locking of the screen failed */
#define STROKE_LOCK_FAILED_R	125.0
#define STROKE_LOCK_FAILED_G	51.0
#define STROKE_LOCK_FAILED_B	0.0

/* Outer circle color while idle */
#define STROKE_AUTH_IDLE_R	51.0
#define STROKE_AUTH_IDLE_G	125.0
#define STROKE_AUTH_IDLE_B	0.0

/* Outer circle color when there is nothing to delete */
#define STROKE_NOTHING_TO_DELETE_R      125.0
#define STROKE_NOTHING_TO_DELETE_G      51.0
#define STROKE_NOTHING_TO_DELETE_B      0.0
