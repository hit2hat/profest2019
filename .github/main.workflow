workflow "CI/CD" {
  on = "push"
  resolves = ["Deploy"]
}

action "Install" {
  uses = "borales/actions-yarn@master"
  args = "install"
}

action "Build" {
  needs = "Install"
  uses = "borales/actions-yarn@master"
  args = "build"
}

action "Deploy" {
  needs = "Build"
  uses = "w9jds/firebase-action@master"
  args = "deploy"
  secrets = ["FIREBASE_TOKEN"]
  env = {
    PROJECT_ID = "alphamusic-studio"
  }
}
