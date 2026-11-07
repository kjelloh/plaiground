# Useful Git Commands

```#ece79d83```

## Revert modified tracked files to the last commit:

```sh
git restore .
```

If some changes are also staged, use git restore --staged --worktree . to unstage and discard them in one go.

NOTE: git restore has no --dry-run option. But since it only discards changes to tracked files, you can preview exactly what it would throw away:

```sh
git status            # lists which files would be affected
git diff              # shows the unstaged changes that would be discarded
git diff --staged     # shows staged changes (relevant if you use --staged)
git diff --stat       # compact summary: files and lines changed
```

## Remove untracked files and directories:

```sh
git clean -n -d    # dry run: shows what would be deleted
git clean -f -d    # actually deletes them
```