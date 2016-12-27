http://ffmpeg.org/releases/ffmpeg-3.2.tar.xz

origin  git@github.com:xkfz007/ffmpeg_git.git (fetch)
origin  git@github.com:xkfz007/ffmpeg_git.git (push)
upstream        https://github.com/FFmpeg/FFmpeg.git (fetch)
upstream        https://github.com/FFmpeg/FFmpeg.git (push)

origin  git@github.com:xkfz007/ffmpeg_git.git (fetch)
origin  git@github.com:xkfz007/ffmpeg_git.git (push)
upstream        https://git.ffmpeg.org/ffmpeg.git (fetch)
upstream        https://git.ffmpeg.org/ffmpeg.git (push)



git remote add upstream [url]
git remote -v

git fetch upstream

git merge upstream/master

git config http.proxy http://127.0.0.1:808
git config --unset http.proxy


