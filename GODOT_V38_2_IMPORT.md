# Godot v38.2 import staging

This branch is reserved for the **complete** `Moonfall_38_2_Checkpoint_Upgrade.zip` project import. It does not yet contain the 3D project. Do **not** merge until source, binary art assets and CI are uploaded and verified.

To import on a Windows computer with Git installed:

1. Download the v38.2 ZIP and extract it.
2. Clone `https://github.com/ericgantor-cyber/Moonfall.git` and checkout `import/moonfall-godot-v38-2`.
3. Copy the contents of the extracted `moonfall38_work` directory into a **new** `godot-v38/` folder inside the clone. Keep the source project and original hero assets unchanged.
4. Run `git add godot-v38` and `git commit -m "Import Moonfall Godot v38.2 full source"` and `git push origin import/moonfall-godot-v38-2`.
5. Run Godot and all bundled tests; open a PR only after checking the imported project and installer build.

The browser-based game on `main` remains unchanged.
