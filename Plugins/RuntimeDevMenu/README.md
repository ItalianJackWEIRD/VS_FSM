# Runtime Dev Menu

An in-game developer menu for Unreal Engine 5.6. Toggle bools and call functions while you play, with a gamepad or a keyboard. Rows are set up in the Details panel by picking a bool or a function from a dropdown: no widgets to build, no input assets to import.

The menu never pauses the game. While it's open it takes only the right stick and the confirm button, so you can keep moving and using every other input.

## Quick start

1. Copy the `RuntimeDevMenu` folder into your project's `Plugins` folder and build.
2. Add the **Dev Menu** component to your player Character (or PlayerController).
3. In **Details > Dev Menu > Entries**, add rows:
   - **Bool / Function**: pick a binding from the dropdown. `Self` is the actor that owns the menu, the other names are its components. Bools show ON/OFF, functions end with `()`.
   - **Pause Game**: pauses and resumes the game.
   - **Separator**: a header line that groups the rows below it.
4. Play and press **Select** on the gamepad or **F10** on the keyboard.

| Action | Gamepad | Keyboard |
|---|---|---|
| Open / close | Select (View) | F10 |
| Move | Right stick | Up / Down |
| Run the row | A (bottom face button) | Enter |

Every key can be changed in **Details > Dev Menu > Input**.

## What the dropdown lists

- Bools and no-parameter functions of the owner (`Self`) and of each of its components, C++ or Blueprint.
- Blueprint custom events with no inputs are functions too, so they can be called from the menu.
- Engine members (Actor, Pawn, CharacterMovement...) are hidden. Turn on **List Engine Members** (advanced) to show them.
- C++ members must be `UPROPERTY` / `UFUNCTION`, and C++ components must be `UPROPERTY` variables of the owner.

## Adding rows from code or Blueprint

Rows from the Details panel come first, rows added at runtime come after.

Blueprint (for example on BeginPlay):

- **Add Toggle** (Label, Target, Bool Name)
- **Add Function** (Label, Target, Function Name)
- **Add Action** (Label, Action): drag from the Action pin and pick *Create Event*.
- **Add Separator**, **Clear Runtime Rows**, **Open Menu**, **Close Menu**, **Is Menu Open**

C++:

```cpp
DevMenu->AddToggle(TEXT("Show debug"), this, GET_MEMBER_NAME_CHECKED(AMyCharacter, bShowDebug));
DevMenu->AddFunction(TEXT("Respawn"), this, GET_FUNCTION_NAME_CHECKED(AMyCharacter, Respawn));
DevMenu->AddLambda(TEXT("God mode"), [this]{ bGodMode = !bGodMode; }, [this]{ return bGodMode; });
```

## How it stays out of gameplay

The menu creates its own Enhanced Input actions and mapping contexts at runtime, at priority 10000:

- the toggle context is always active and holds only the toggle keys;
- the menu context is added when the menu opens and removed when it closes. Its actions consume their keys, so lower-priority gameplay mappings on the right stick and the confirm button are ignored while the menu is open.

The menu is drawn with Slate on top of the viewport and never takes focus or mouse input. It keeps working while the game is paused.

## Notes

- Requires the Enhanced Input plugin (enabled by default in UE 5).
- Disabled in Shipping builds unless **Enable In Shipping** is on.
- Local players only; on a dedicated server the component does nothing.
- If a binding can't be found at runtime the row is shown in red with *missing*, and the Output Log says why.

## Credits

Font: JetBrains Mono, © The JetBrains Mono Project Authors, SIL Open Font License 1.1 (`Resources/Fonts/OFL.txt`).
