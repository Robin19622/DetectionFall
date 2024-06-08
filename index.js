/**
 * @format
 */

import { AppRegistry } from 'react-native';
import App from './App';  // Importe le composant principal App de l'application
import { name as appName } from './app.json';  // Récupère le nom de l'application

// Enregistrement du composant projet
AppRegistry.registerComponent('FallDetection', () => App);

