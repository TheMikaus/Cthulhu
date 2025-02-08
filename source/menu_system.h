struct MenuSystemNode
{
    const char* text;
    const char* confirmationPrompt;
    void (*callback)();
    MenuSystemNode* parentMenu;
    MenuSystemNode* subMenu;
    MenuSystemNode* previousSibling;
    MenuSystemNode* nextSibling;
};

struct MenuSystem
{
    MenuSystemNode* topMenuRoot = nullptr;
    MenuSystemNode* currentRoot = nullptr;
    MenuSystemNode* currentSelectedItem = nullptr;
    MenuSystemNode* lastItem = nullptr;

    const u8 startingDisplayIndex = 3;
    u8 currentSelectedDisplayIndex = startingDisplayIndex;
};

u32 waitKey() {
    u32 kDown = 0;
    while (aptMainLoop()) {
        hidScanInput();
        kDown = hidKeysDown();
        if (kDown) break;
        gfxEndFrame();
    }
    consoleClear();
    return kDown;
}

bool promptConfirm(const char* title, const char* message)
{
    consoleClear();
    printf("\x1b[1;0H\x1b[30;47m%-50s", " ");
    printf("\x1b[1;%uH%s\x1b[0;0m", (25 - (strlen(title) >> 1)), title);
    printf("\x1b[14;%uH%s", (25 - (strlen(message) >> 1)), message);
    printf("\x1b[16;14H\x1b[32m(A)\x1b[37m Confirm / \x1b[31m(B)\x1b[37m Cancel");
    u32 kDown = waitKey();
    return (kDown & KEY_A);
}

void promptError(const char* title, const char* message)
{
    consoleClear();
    printf("\x1b[1;0H\x1b[30;47m%-50s", " ");
    printf("\x1b[1;%uH%s\x1b[0;0m", (25 - (strlen(title) >> 1)), title);
    printf("\x1b[14;%uH%s", (25 - (strlen(message) >> 1)), message);
    waitKey();
}

void InitializeMenuSystem(MenuSystem& menuSystem, MenuSystemNode* mainMenuRoot)
{
    menuSystem.topMenuRoot = menuSystem.currentRoot = menuSystem.currentSelectedItem = mainMenuRoot;
}

void MenuAssignParentAndSiblings(MenuSystemNode* parent, MenuSystemNode* root)
{
    MenuSystemNode* currentMenuItem = root;
    MenuSystemNode* previousMenuItem = nullptr;
    while (currentMenuItem)
    {
        currentMenuItem->previousSibling = previousMenuItem;
        currentMenuItem->parentMenu = parent;
        previousMenuItem = currentMenuItem;
        currentMenuItem = currentMenuItem->nextSibling;
    }
}

void MenuGoBack(MenuSystemNode** currentRoot, MenuSystemNode** currentSelectedItem)
{
    if ((*currentRoot)->parentMenu)
    {
        *currentRoot = *currentSelectedItem = (*currentRoot)->parentMenu;
        while ((*currentRoot)->previousSibling)
        {
            *currentRoot = (*currentRoot)->previousSibling;
        }
    }
}

void MenuDisplayHeader(MenuSystem& menuSystem, u8& displayIndex)
{
    // Display the menu header
    const char* menuHeader = (menuSystem.currentRoot->parentMenu) ? menuSystem.currentRoot->parentMenu->text : "Main menu";
    printf("\x1b[%u;0H%-48s", displayIndex++, menuHeader);
    printf("\x1b[%u;0H================================================", displayIndex++);
    displayIndex += 2;
}

void MenuDisplayCurrentOptions(MenuSystem& menuSystem, MenuSystemNode* currentMenuItem, u8& displayIndex)
{
    // Display current menu
    while (currentMenuItem != nullptr)
    {
        printf("\x1b[%u;4H%-48s", displayIndex, currentMenuItem->text);
        if (currentMenuItem == menuSystem.currentSelectedItem)
        {
            menuSystem.currentSelectedDisplayIndex = displayIndex;
        }
        menuSystem.lastItem = currentMenuItem;
        currentMenuItem = currentMenuItem->nextSibling;
        displayIndex++;
    }
}

void MenuDisplayGoBackOption(MenuSystem& menuSystem, u8& displayIndex)
{
        // Add an extra line, and display the go back option
        displayIndex++;
        printf("\x1b[%u;4H%-48s", displayIndex, menuSystem.currentRoot->parentMenu ? "Go back." : "Exit Application");
}

void MenuDisplayBottomScreenDebug(PrintConsole& topScreen, PrintConsole& bottomScreen, MenuSystem& menuSystem)
{
    consoleSelect(&bottomScreen);
    printf("\x1b[1;2HSelected -> %-28s", (menuSystem.currentSelectedItem) ? menuSystem.currentSelectedItem->text : "Go Back");
    printf("\x1b[2;2HRoot -> %-28s", menuSystem.currentRoot->text);
    consoleSelect(&topScreen);
}

void MenuHandleUpDownMenuNavigation(u32 kDown, MenuSystem& menuSystem)
{
    // Handle up/down menu navigation
    if (kDown & KEY_DOWN)
    {
        // Clear the Carret
        printf("\x1b[%u;2H ", menuSystem.currentSelectedDisplayIndex);
        if (menuSystem.currentSelectedItem == nullptr) 
        {
            menuSystem.currentSelectedItem = menuSystem.currentRoot;
        }
        else
        {
            menuSystem.currentSelectedItem = menuSystem.currentSelectedItem->nextSibling;
        }
    }
    else if (kDown & KEY_UP)
    {
        // Clear the Carret
        printf("\x1b[%u;2H ", menuSystem.currentSelectedDisplayIndex);
        if (menuSystem.currentSelectedItem == nullptr)
        {
            menuSystem.currentSelectedItem = menuSystem.lastItem;
        }
        else
        {
            menuSystem.currentSelectedItem = menuSystem.currentSelectedItem->previousSibling;
        }
    }
}

void MenuHandleMenuSelectionOrBackout(u32 kDown, MenuSystem& menuSystem)
{
    // handle selection and backout
    if (kDown & KEY_A)
    {
        // if currentSelectedItem is nullptr then we are on the "go back" item
        if (menuSystem.currentSelectedItem == nullptr)
        {
            MenuGoBack(&menuSystem.currentRoot, &menuSystem.currentSelectedItem);
            consoleClear();
        }
        else
        {
            // if there is a subMenu then go into it, otherwise prompt and execute.
            if (menuSystem.currentSelectedItem->subMenu)
            {
                menuSystem.currentRoot = menuSystem.currentSelectedItem->subMenu;
                menuSystem.currentSelectedItem = menuSystem.currentRoot;
                consoleClear();
            }
            else
            {
                if (menuSystem.currentSelectedItem->confirmationPrompt == nullptr || promptConfirm(menuSystem.currentSelectedItem->text, menuSystem.currentSelectedItem->confirmationPrompt))
                {
                    if (menuSystem.currentSelectedItem->callback)
                    {
                        // Fix this for unwrap (it needs a false)
                        menuSystem.currentSelectedItem->callback();
                    }
                    consoleClear();
                }
            }
        }
    }
    else if (kDown & KEY_B)
    {
        if (menuSystem.currentRoot->parentMenu)
        {
            MenuGoBack(&menuSystem.currentRoot, &menuSystem.currentSelectedItem);
            consoleClear();
        }
    }

}

void TickMenu(MenuSystem& menuSystem, u32 kDown)
{
    MenuSystemNode* currentMenuItem = menuSystem.currentRoot;
    u8 displayIndex = menuSystem.startingDisplayIndex;
    MenuDisplayHeader(menuSystem, displayIndex);
    MenuDisplayCurrentOptions(menuSystem, currentMenuItem, displayIndex);
    MenuDisplayGoBackOption(menuSystem, displayIndex);

    // If the currently selected item is not assigned move us to the top of the list
    if (menuSystem.currentSelectedItem == nullptr)
    {
        menuSystem.currentSelectedDisplayIndex = displayIndex;
    }

    // Put the cursor in front of the selected item
    printf("\x1b[%u;2H>", menuSystem.currentSelectedDisplayIndex);

    MenuDisplayBottomScreenDebug(topScreen, bottomScreen, menuSystem);

    MenuHandleUpDownMenuNavigation(kDown, menuSystem);
    MenuHandleMenuSelectionOrBackout(kDown, menuSystem);
}