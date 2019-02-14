/*
 * Copyright (c) 2019 pinc Software. All Rights Reserved.
 */
#ifndef COVER_WINDOW_H
#define COVER_WINDOW_H


#include <Window.h>


class BGridView;
class BTextControl;


class CoverWindow : public BWindow {
public:
								CoverWindow();
	virtual						~CoverWindow();

	virtual	void				MessageReceived(BMessage* message);

private:
			BGridView*			fMainView;
			BTextControl*		fArtistControl;
			BTextControl*		fTitleControl;
			void*				fQuery;
};


#endif // COVER_WINDOW_H
