var gulp = require('gulp');
var minifyjs = require('gulp-uglify');
var minifycss = require('gulp-clean-css');
var rename = require("gulp-rename");

gulp.task('compile', ['compile-css','compile-js']);

gulp.task('compile-js', function(){
  gulp.src("js/_unminified/*.js")
  .pipe(minifyjs())
  .pipe(rename({
          suffix: '.min'
        }))
  .pipe(gulp.dest("js"));
});

gulp.task('compile-css', function(){
  gulp.src("css/_unminified/*.css")
  .pipe(minifycss())
  .pipe(rename({
          suffix: '.min'
        }))
  .pipe(gulp.dest("css"));
});
