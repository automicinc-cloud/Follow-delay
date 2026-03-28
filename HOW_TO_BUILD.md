# How To Get Your Follow Delay VST3 (No Dev Tools Needed)

## One-Time Setup (~5 minutes)

### 1. Create a GitHub account
Go to https://github.com/signup (free)

### 2. Create a new repository
- Click the **+** button (top right) → **New repository**
- Name it `follow-delay`
- Set it to **Public** (free CI minutes)
- Click **Create repository**

### 3. Upload the project files
- On your new repo page, click **"uploading an existing file"**
- Drag the entire contents of the **FollowDelay** folder into the upload area:
  - `Source/` folder
  - `CMakeLists.txt`
  - `README.md`
  - `.github/` folder (this contains the build automation)
- Scroll down and click **Commit changes**

### 4. Wait for it to build
- Click the **Actions** tab at the top of your repo
- You'll see "Build Follow Delay VST3" running (yellow dot)
- Wait ~3-5 minutes for it to finish (green checkmark)

### 5. Download your plugin
- Click on the completed build run
- Scroll to the bottom — you'll see **FollowDelay-VST3-Windows** under Artifacts
- Click it to download a .zip file

### 6. Install in REAPER
- Unzip the download
- Copy the `Follow Delay.vst3` folder into:
  ```
  C:\Program Files\Common Files\VST3\
  ```
- Open REAPER → Options → Preferences → Plug-ins → VST → **Re-scan**
- Add to your guitar track: **FX → Follow Delay**

---

## Updating the Plugin Later

If I make changes to the code for you:
1. Download the updated files
2. Go to your GitHub repo
3. Upload the changed files (GitHub will ask to overwrite)
4. Commit — the build runs automatically
5. Download the new VST3 from Actions → Artifacts

That's it. No compilers, no terminal, no dev tools.
