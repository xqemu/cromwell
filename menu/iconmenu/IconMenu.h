#ifndef _ICONMENU_H_
#define _ICONMENU_H_

/***************************************************************************
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 ***************************************************************************/

#define ICON_SOURCE_SLOT0 0
#define ICON_SOURCE_SLOT1 ICON_WIDTH
#define ICON_SOURCE_SLOT2 ICON_WIDTH*2
#define ICON_SOURCE_SLOT3 ICON_WIDTH*3
#define ICON_SOURCE_SLOT4 ICON_WIDTH*4
#define ICON_SOURCE_SLOT5 ICON_WIDTH*5
#define ICON_SOURCE_SLOT6 ICON_WIDTH*6
#define ICON_SOURCE_SLOT7 ICON_WIDTH*7
#define ICON_SOURCE_SLOT8 ICON_WIDTH*8

#define MAX_ICON_TITLE 18

struct ICON;

typedef struct ICON {
	int iconSlot;
	char *szCaption;
	void (*functionPtr) (void *);
	void *functionDataPtr;
	struct ICON *previousIcon;
	struct ICON *nextIcon;
} ICON;


extern ICON *selectedIcon;

//If this is set, the menu item was chosen automatically, due to timeout
extern int timedOut;

//Adds a new icon into the menu - they are displayed in the order added.
void AddIcon(ICON *newIcon);

//This draws and handles input for the main menu
void IconMenu(void);

#endif
