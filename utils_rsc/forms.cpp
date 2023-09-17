#include "forms.h"

#define LLOBT(x) ((int8_t)(x))

#define OB_NEXT(tree, id) tree[id].ob_next
#define OB_HEAD(tree, id) tree[id].ob_head
#define OB_TAIL(tree, id) tree[id].ob_tail
#define OB_TYPE(tree, id) (tree[id].ob_type & 0xff)
#define OB_EXTYPE(tree, id) (tree[id].ob_type >> 8)
#define OB_FLAGS(tree, id) tree[id].ob_flags
#define OB_STATE(tree, id) tree[id].ob_state
#define OB_SPEC(tree, id) tree[id].ob_spec
#define OB_X(tree, id) tree[id].ob_x
#define OB_Y(tree, id) tree[id].ob_y
#define OB_WIDTH(tree, id) tree[id].ob_width
#define OB_HEIGHT(tree, id) tree[id].ob_height

#define M1_ENTER 0x0000
#define M1_EXIT 0x0001
#define BS 0x0008
#define TAB 0x0009
#define CR 0x000D
#define ESC 0x001B
#define BTAB 0x0f00
#define UP 0x4800
#define DOWN 0x5000
#define DEL 0x5300

GRECT br_rect;                 /* Current break rectangle  */
int16_t br_mx, br_my, br_togl; /* Break mouse posn & flag  */
int16_t fn_object;             /* Found tabable object     */
int16_t fn_last;               /* Object tabbing from      */
int16_t fn_prev;               /* Last EDITABLE obj seen   */
int16_t fn_dir;                /* 1 = TAB, 0 = BACKTAB     */

/************* Utility routines for new forms manager ***************/

/* Return the object's GRECT  	*/
/* through 'p'		        */
void new_objc_xywh(OBJECT *tree, int16_t obj, GRECT *p)
{
    objc_offset(tree, obj, &p->g_x, &p->g_y);
    p->g_w = OB_WIDTH(tree, obj);
    p->g_h = OB_HEIGHT(tree, obj);
}

/* Reverse the SELECT state */
/* of an object, and redraw */
/* it immediately.          */
void objc_toggle(OBJECT *tree, int16_t obj)
{
    int16_t state, newstate;
    GRECT rect_root, ob_rect;

    new_objc_xywh(tree, ROOT, &rect_root);

    state = OB_STATE(tree, obj);
    newstate = state ^ SELECTED;

    objc_change(tree, obj, 0, rect_root.g_x, rect_root.g_y, rect_root.g_w, rect_root.g_h, newstate, 1);
}

/* If the object is not already */
/* SELECTED, make it so.	    */
void objc_sel(OBJECT *tree, int16_t obj)
{
    if (!(OB_STATE(tree, obj) & SELECTED))
    {
        objc_toggle(tree, obj);
    }
}

/* If the object is SELECTED,	*/
/* deselect it.			        */
void objc_dsel(OBJECT *tree, int16_t obj)
{
    if ((OB_STATE(tree, obj) & SELECTED))
    {
        objc_toggle(tree, obj);
    }
}

/* Non-cursive traverse of an	*/
/* object tree.  This routine	*/
/* is described in PRO GEM #5.	*/
void map_tree(OBJECT *tree, int16_t this_ob, int16_t last, int16_t (*routine)(OBJECT *tree, int16_t this_ob))
{
    int16_t tmp1;
    /* Initialize to impossible value: */
    /* TAIL won't point to self!	   */
    /* Look until final node, or off   */
    /* the end of tree		           */
    tmp1 = this_ob;

    while (this_ob != last && this_ob != NIL)
    {
        /* Did we 'pop' into this_ob node	   */
        /* for the second time?	        	   */
        if (OB_TAIL(tree, this_ob) != tmp1)
        {
            /* this is a new node       */
            tmp1 = this_ob;
            this_ob = NIL;
            /* Apply operation, testing  */
            /* for rejection of sub-tree */
            if ((*routine)(tree, tmp1))
            {
                this_ob = OB_HEAD(tree, tmp1);
            }
            /* Subtree path not taken,   */
            /* so traverse right	     */
            if (this_ob == NIL)
            {
                this_ob = OB_NEXT(tree, tmp1);
            }
        }
        else
        {
            /* Revisiting parent: 	     */
            /* No operation, move right  */
            tmp1 = this_ob;
            this_ob = OB_NEXT(tree, tmp1);
        }
    }
}

/* Find the parent object of 	*/
/* by traversing right until	*/
/* we find nodes whose NEXT	    */
/* and TAIL linkey_special_status point to 	    */
/* each other.			        */
int16_t get_parent(OBJECT *tree, int16_t obj)
{
    int16_t pobj;

    if (obj == NIL)
    {
        return (NIL);
    }
    pobj = OB_NEXT(tree, obj);
    if (pobj != NIL)
    {
        while (OB_TAIL(tree, pobj) != obj)
        {
            obj = pobj;
            pobj = OB_NEXT(tree, obj);
        }
    }
    return (pobj);
}

/* determine if x,y is in rectangle	*/
int16_t inside(int16_t x, int16_t y, GRECT *pt)
{
    if ((x >= pt->g_x) && (y >= pt->g_y) && (x < pt->g_x + pt->g_w) && (y < pt->g_y + pt->g_h))
        return (TRUE);
    else
        return (FALSE);
}

/************* "Hot-spot" manager and subroutines  ***************/

/* Breaking object is right of	*/
/* mouse.  Reduce width of 	    */
/* bounding rectangle.		    */
int16_t break_x(int16_t *pxy){
    if (br_mx < pxy[0])
    {
        br_rect.g_w = pxy[0] - br_rect.g_x;
        return (TRUE);
    }
    /* Object to left.  Reduce width    */
    /* and move rect. to right	        */
    if (br_mx > pxy[2])
    {
        br_rect.g_w += br_rect.g_x - pxy[2] - 1;
        br_rect.g_x = pxy[2] + 1;
        return (TRUE);
    }
    /* Mouse within object segment.	*/
    /* Break attempt fails.		    */
    return (FALSE);
}

int16_t break_y(int16_t *pxy){
    /* Object below mouse.  Reduce	*/
    /* height of bounding rect.     */
    if (br_my < pxy[1])
    {
        br_rect.g_h = pxy[1] - br_rect.g_y;
        return (TRUE);
    }
    /* Object above mouse.  Reduce	*/
    /* height and shift downward.	*/
    if (br_my > pxy[3])
    {
        br_rect.g_h += br_rect.g_y - pxy[3] - 1;
        br_rect.g_y = pxy[3] + 1;
        return (TRUE);
    }
    /* Emergency escape test! Protection vs. turkeys who nest */
    /* non-selectable objects inside of selectables.          */
    if (br_mx >= pxy[0] && br_mx <= pxy[1])
    {
        /* Will X break fail?   */
        /* If so, punt!         */
        br_rect.g_x = br_mx;
        br_rect.g_y = br_my;
        br_rect.g_w = br_rect.g_h = 1;
        return (TRUE);
    }
    return (FALSE);
}

/* Called once per object to	*/
/* check if the bounding rect.	*/
/* needs to be modified.	    */
int16_t break_object(OBJECT *tree, int16_t obj){
    GRECT s;
    int16_t flags, broken, pxy[4];

    new_objc_xywh(tree, obj, &s);
    grect_to_array(&s, pxy);
    /* Trivial rejection case 	*/
    if (!rc_intersect(&br_rect, &s))
    {
        return (FALSE);
    }
    /* Is this object a potential	*/
    /* hot-spot?		     	    */
    flags = OB_FLAGS(tree, obj);
    if (flags & OF_HIDETREE)
    {
        return (FALSE);
    }
    if (!(flags & OF_SELECTABLE))
    {
        return (TRUE);
    }
    if (OB_STATE(tree, obj) & OS_DISABLED)
    {
        return (TRUE);
    }
    /* This could take two passes 	*/
    /* if the first break fails.   	*/
    for (broken = FALSE; !broken;)
    {
        if (br_togl)
        {
            broken = break_x(pxy);
        }
        else
        {
            broken = break_y(pxy);
        }
        br_togl = !br_togl;
    }
    return (TRUE);
}

/* Manages mouse rectangle events */
int16_t form_hotspot(OBJECT *tree, int16_t hotspot_object, int16_t mouse_x, int16_t mouse_y, GRECT *rect, int16_t *mode){
    GRECT rect_root;
    int16_t state;

    new_objc_xywh(tree, ROOT, &rect_root);

    /* If there is already a hot-spot */
    /* turn it off.			        */
    if (hotspot_object != NIL)
    {
        objc_toggle(tree, hotspot_object);
    }

    /* Mouse has moved outside of 	  */
    /* the dialog.  Wait for return.  */
    if (!(inside(mouse_x, mouse_y, &rect_root)))
    {
        *mode = M1_ENTER;
        rc_copy(&rect_root, rect);
        return (NIL);
    }

    /* What object is mouse over?	  */
    /* (Hit is guaranteed.)           */
    hotspot_object = objc_find(tree, ROOT, MAX_DEPTH, mouse_x, mouse_y);
    /* Is this object a hot-spot?	  */
    state = OB_STATE(tree, hotspot_object);
    if (OB_FLAGS(tree, hotspot_object) & OF_SELECTABLE)
    {
        if (!(state & OS_DISABLED))
        {
            /* Yes!  Set up wait state.	  */
            *mode = M1_EXIT;
            new_objc_xywh(tree, hotspot_object, rect);
            /* But only toggle if it's not	  */
            /* already SELECTED!		      */
            if (state & SELECTED)
            {
                return (NIL);
            }
            else
            {
                objc_toggle(tree, hotspot_object);
                return (hotspot_object);
            }
        }
    }

    /* No hot object, so compute	*/
    /* mouse bounding rectangle.	*/
    rc_copy(&rect_root, &br_rect);
    br_mx = mouse_x;
    br_my = mouse_y;
    br_togl = 0;
    map_tree(tree, ROOT, NIL, break_object);
    rc_copy(&br_rect, rect);

    /* Then return to wait state.	*/
    *mode = M1_EXIT;

    return (NIL);
}

/************* Keyboard manager and subroutines ***************/

/* Check if the object is DEFAULT	*/
int16_t objc_find_default(OBJECT *tree, int16_t obj){
    /* Is sub-tree hidden?			*/
    if (OF_HIDETREE & OB_FLAGS(tree, obj))
    {
        return (FALSE);
    }
    /* Must be DEFAULT and not DISABLED	*/
    if (OF_DEFAULT & OB_FLAGS(tree, obj))
        if (!(OS_DISABLED & OB_STATE(tree, obj)))
        {
            /* Record object numouse_buttoner			*/
            fn_object = obj;
        }
    return (TRUE);
}

/* Look for target of TAB operation.	*/
int16_t find_tab(OBJECT *tree, int16_t obj){
    /* Check for hiddens subtree.		*/
    if (OF_HIDETREE & OB_FLAGS(tree, obj))
    {
        return (FALSE);
    }
    /* If not EDITABLE, who cares?		*/
    if (!(OF_EDITABLE & OB_FLAGS(tree, obj)))
    {
        return (TRUE);
    }
    /* Check for forward tab match		*/
    if (fn_dir && fn_prev == fn_last)
    {
        fn_object = obj;
    }
    /* Check for backward tab match		*/
    if (!fn_dir && obj == fn_last)
    {
        fn_object = fn_prev;
    }
    /* Record object for next call.		*/
    fn_prev = obj;
    return (TRUE);
}

int16_t new_form_keybd(OBJECT *tree, int16_t edit_object, int16_t next_object, int16_t key_scancode, int16_t *out_object, int16_t *okey_scancode){
    /* If lower byte valid, mask out	*/
    /* extended code byte.			    */
    if (LLOBT(key_scancode))
    {
        key_scancode &= 0xff;
    }
    /* Default tab direction if backward.	*/
    fn_dir = 0;
    switch (key_scancode)
    {
    /* Zap character.       */
    case CR:
        *okey_scancode = 0;
        /* Look for a DEFAULT object.		*/
        fn_object = NIL;
        map_tree(tree, ROOT, NIL, objc_find_default);
        /* If found, SELECT and force exit.	*/
        if (fn_object != NIL)
        {
            objc_sel(tree, fn_object);
            *out_object = fn_object;
            return (FALSE);
        }
        /* Falls through to 	*/
        /* tab if no default 	*/
    case TAB:
    case DOWN:
        /* Set fwd direction 	*/
        fn_dir = 1;
    case BTAB:
    case UP:
        /* Zap character	*/
        *okey_scancode = 0;
        fn_last = edit_object;
        /* Look for TAB object	*/
        fn_prev = fn_object = NIL;
        map_tree(tree, ROOT, NIL, find_tab);
        /* try to wrap around 	*/
        if (fn_object == NIL)
        {
            map_tree(tree, ROOT, NIL, find_tab);
        }
        if (fn_object != NIL)
        {
            *out_object = fn_object;
        }
        break;
    default:
        /* Pass other chars	*/
        return (TRUE);
    }
    return (TRUE);
}

/************* Mouse button manager and subroutines ***************/

void do_radio(OBJECT *tree, int16_t obj)
{
    GRECT rect_root;
    int16_t pobj, sobj, state;

    new_objc_xywh(tree, ROOT, &rect_root);

    /* Get the object's parent */
    pobj = get_parent(tree, obj);

    for (sobj = OB_HEAD(tree, pobj); sobj != pobj; sobj = OB_NEXT(tree, sobj))
    {
        /* Deselect all but...	   */
        if (sobj != obj){
            objc_dsel(tree, sobj);
        }
    }
    /* the one being SELECTED  */
    objc_sel(tree, obj);
}

/* Mouse button handler	   */
int16_t new_form_button(OBJECT *tree, int16_t obj, int16_t clicks, int16_t *next_object, int16_t *hotspot_object)
{
    int16_t flags, state, hibit, texit, sble, dsbld, edit;
    int16_t in_out, in_state;

    /* Get flags and states   */
    flags = OB_FLAGS(tree, obj);
    state = OB_STATE(tree, obj);

    texit = flags & OF_TOUCHEXIT;
    sble = flags & OF_SELECTABLE;
    dsbld = state & OS_DISABLED;
    edit = flags & OF_EDITABLE;

    /* This is not an  	*/
    /* interesting object	*/
    if (!texit && (!sble || dsbld) && !edit)
    {
        *next_object = 0;
        return (TRUE);
    }

    /* Preset special flag	*/
    if (texit && clicks == 2)
    {
        hibit = 0x8000;
    }
    else
    {
        hibit = 0x0;
    }
    /* Hot stuff!		*/
    if (sble && !dsbld)
    {
        /* Process radio buttons    */
        /* immediately!		        */
        if (flags & OF_RBUTTON)
        {
            do_radio(tree, obj);
        }
        else if (!texit)
        {
            /* Already toggled ? */
            in_state = (obj == *hotspot_object) ? state : state ^ SELECTED;
            if (!graf_watchbox(tree, obj, in_state, in_state ^ SELECTED))
            {
                /* He gave up...  */
                *next_object = 0;
                *hotspot_object = NIL;
                return (TRUE);
            }
        }
        /* if (texit) */
        else
        {
            if (obj != *hotspot_object) /* Force SELECTED	*/
                objc_toggle(tree, obj);
        }
    }

    /* We're gonna do it! So don't	*/
    /* turn it off later.		*/
    if (obj == *hotspot_object)
    {
        *hotspot_object = NIL;
    }
    /* Exit conditions.		*/
    if (texit || (flags & OF_EXIT))
    {
        *next_object = obj | hibit;
        /* Time to leave!		*/
        return (FALSE);
    }
    else
    {
        if (!edit)
        {
            /* Clear object unless tabbing	*/
            *next_object = 0;
        }
    }
    return (TRUE);
}
