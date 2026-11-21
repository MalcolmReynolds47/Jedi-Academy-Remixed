@echo off
"%~dp0JediAcademyRemixedMP\jar_server.x86_64.exe" +set fs_basepath "%~dp0." +set fs_game JediAcademyRemixedMP +set dedicated 2 +exec jar-server-example.cfg +exec server.cfg %*
